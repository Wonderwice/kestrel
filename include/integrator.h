#pragma once

#include "pcg32.h"
#include "ray.h"
#include "vec3.h"
#include <functional>
#include <vector>

namespace kestrel {

class Scene;
class Camera;

class Integrator {
public:
  virtual ~Integrator() = default;
  virtual Color integrate(const Ray &ray, const Scene &scene, PCG32 &rng) const = 0;

  /**
   * @brief Render the scene into a pixel buffer using multithreading.
   * @param scene             Scene to render (already built).
   * @param camera            Camera through which to render.
   * @param samples_per_pixel Samples per pixel for anti-aliasing.
   * @param pixels            Output buffer, size = camera.width()*camera.height().
   * @param num_threads       Number of worker threads.
   * @param on_progress       Optional callback invoked with (rows_done,
   *                          total_rows) as rendering progresses; the final
   *                          call satisfies rows_done == total_rows. Used by
   *                          the CLI to draw a progress bar; the library itself
   *                          performs no terminal I/O.
   *
   * Divides the image into scanlines handed out atomically to the worker
   * threads, driving the virtual integrate() once per sample.
   */
  void render(const Scene &scene, const Camera &camera, int samples_per_pixel,
              std::vector<Color> &pixels, int num_threads,
              const std::function<void(int done, int total)> &on_progress = {}) const;

protected:
  // Power heuristic (β=2) for MIS weighting of two sampling strategies.
  static float mis_power(float a, float b) {
    float a2 = a * a, b2 = b * b;
    return a2 / (a2 + b2 + 1e-10f);
  }
};

}  // namespace kestrel
