/**
 * @file integrator.cpp
 * @brief Shared render loop for all integrators.
 *
 * Implements Integrator::render: the multithreaded sampling loop that was
 * previously the CLI's render_scene. It divides the image into scanlines,
 * hands them out atomically to worker threads, and drives the virtual
 * integrate() once per sample. Progress is reported through an optional
 * callback so the library performs no terminal I/O of its own.
 */

#include "integrator.h"
#include "camera.h"
#include "scene.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace kestrel {

void Integrator::render(const Scene &scene, const Camera &camera,
                        int samples_per_pixel, std::vector<Color> &pixels,
                        int num_threads,
                        const std::function<void(int done, int total)> &on_progress) const {

  const int W = camera.width(), H = camera.height();

  // Gaussian reconstruction filter, matching Mitsuba's default `gaussian` rfilter:
  // stddev 0.5, support radius = 4·stddev = 2px, truncated so the weight reaches
  // zero at the edge. Each sample is splatted onto every pixel whose centre lies
  // within the radius (separable weight), and each pixel is finally normalised by
  // its accumulated filter weight. This anti-aliases edges and high-frequency
  // texture detail (e.g. the checker floor) the way Mitsuba does — a box filter
  // (one sample → its own pixel) leaves them aliased.
  constexpr float kStddev = 0.5f;
  constexpr float kRadius = 4.0f * kStddev;            // 2 px support
  const float inv_2s2 = 1.0f / (2.0f * kStddev * kStddev);
  const float edge    = std::exp(-kRadius * kRadius * inv_2s2);
  auto filt = [&](float d) {                            // 1-D truncated gaussian
    float w = std::exp(-d * d * inv_2s2) - edge;
    return w > 0.0f ? w : 0.0f;
  };

  std::vector<std::thread> threads;
  std::atomic<int> next_row(0);
  std::atomic<int> completed_rows(0);
  const int total_rows = H;

  PCG32 global_rng;
  std::vector<uint64_t> thread_seeds;
  thread_seeds.reserve(num_threads);
  for (int t = 0; t < num_threads; ++t)
    thread_seeds.push_back(global_rng.next());

  // Per-thread accumulation frames (Σ weight·colour and Σ weight). Splatting
  // writes to neighbouring rows, so each worker owns a private frame to avoid
  // data races; the frames are summed once the workers finish.
  const size_t N = static_cast<size_t>(W) * static_cast<size_t>(H);
  std::vector<std::vector<Color>> accum(num_threads, std::vector<Color>(N, Color(0, 0, 0)));
  std::vector<std::vector<float>> weight(num_threads, std::vector<float>(N, 0.0f));

  auto render_worker = [&](int thread_id) {
    PCG32 thread_rng(thread_seeds[thread_id] + static_cast<uint64_t>(thread_id));
    std::vector<Color> &acc = accum[thread_id];
    std::vector<float>  &wgt = weight[thread_id];
    while (true) {
      int j = next_row.fetch_add(1);
      if (j >= H)
        break;

      for (int i = 0; i < W; ++i) {
        for (int s = 0; s < samples_per_pixel; ++s) {
          // Continuous film position; map to [0,1] with /W,/H so pixel i's centre
          // is (i+0.5)/W (Mitsuba's convention).
          float sx = i + thread_rng.next_float();
          float sy = j + thread_rng.next_float();
          Ray ray = camera.get_ray(sx / W, sy / H, thread_rng);
          Color L = integrate(ray, scene, thread_rng);
          if (std::isnan(L.x) || std::isnan(L.y) || std::isnan(L.z))
            L = Color(0, 0, 0); // Replace NaN with black

          int ax0 = std::max(0,     static_cast<int>(std::ceil (sx - 0.5f - kRadius)));
          int ax1 = std::min(W - 1, static_cast<int>(std::floor(sx - 0.5f + kRadius)));
          int by0 = std::max(0,     static_cast<int>(std::ceil (sy - 0.5f - kRadius)));
          int by1 = std::min(H - 1, static_cast<int>(std::floor(sy - 0.5f + kRadius)));
          for (int b = by0; b <= by1; ++b) {
            float wy = filt(sy - (b + 0.5f));
            if (wy <= 0.0f) continue;
            for (int a = ax0; a <= ax1; ++a) {
              float w = wy * filt(sx - (a + 0.5f));
              if (w <= 0.0f) continue;
              size_t idx = static_cast<size_t>(b) * W + a;
              acc[idx] += L * w;
              wgt[idx] += w;
            }
          }
        }
      }
      completed_rows.fetch_add(1, std::memory_order_relaxed);
    }
  };

  // Progress poller — reports row counts to the caller at 20 Hz. Presentation
  // (terminal output, ETA) is the caller's responsibility via on_progress.
  std::thread progress_thread([&]() {
    while (true) {
      int done = completed_rows.load(std::memory_order_relaxed);
      bool finished = done >= total_rows;

      if (on_progress)
        on_progress(done, total_rows);

      if (finished) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });

  for (int t = 0; t < num_threads; ++t)
    threads.emplace_back(render_worker, t);
  for (auto &th : threads)
    th.join();
  progress_thread.join();

  // Combine the per-thread frames and normalise by accumulated filter weight.
  for (size_t idx = 0; idx < N; ++idx) {
    Color c(0, 0, 0);
    float w = 0.0f;
    for (int t = 0; t < num_threads; ++t) {
      c += accum[t][idx];
      w += weight[t][idx];
    }
    pixels[idx] = (w > 0.0f) ? c * (1.0f / w) : Color(0, 0, 0);
  }
}

}  // namespace kestrel
