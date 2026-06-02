/**
 * @file kestrel.cpp
 * @brief Renderer CLI entry point.
 * @author Alexei Czornyj
 * @date 2026
 *
 * Parses command-line arguments, loads a scene, builds the BVH, runs the
 * selected integrator (path or direct), and writes the result to an EXR image.
 */

#include "io/image_io.h"
#include "kestrel.h"
#include "camera.h"
#include "direct_integrator.h"
#include "integrator.h"
#include "logger.h"
#include "path_integrator.h"
#include "ray.h"
#include "scene.h"
#include "vec3.h"
#include <chrono>
#include <functional>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>

// The renderer's library code lives in namespace kestrel; the CLI driver
// (render_scene + main) stays in the global namespace and pulls it in here.
using namespace kestrel;

/**
 * @brief Build a progress callback that draws a single-line stderr bar.
 * @return Callback taking (rows_done, total_rows); a no-op when stderr is not
 *         a TTY. Captures its own start time for elapsed/ETA reporting.
 *
 * Terminal presentation lives here in the CLI; the library's render loop only
 * reports row counts (see Integrator::render).
 */
static std::function<void(int, int)> make_progress_bar() {
  const bool tty = isatty(STDERR_FILENO);
  if (!tty)
    return {};

  const auto start_time = std::chrono::steady_clock::now();
  constexpr int bar_width = 40;

  return [start_time, bar_width](int done, int total) {
    bool finished = done >= total;

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - start_time).count();
    double pct = static_cast<double>(done) / total;
    double eta = (done > 0) ? elapsed / done * (total - done) : 0.0;

    int filled = static_cast<int>(pct * bar_width);
    char bar[bar_width + 1];
    for (int k = 0; k < bar_width; ++k) {
      if (k < filled)                      bar[k] = '=';
      else if (k == filled && !finished)   bar[k] = '>';
      else                                 bar[k] = ' ';
    }
    bar[bar_width] = '\0';

    std::fprintf(stderr, "\r[%s] %3d%%  %5.1fs elapsed  %5.1fs remaining",
                 bar, static_cast<int>(pct * 100), elapsed, eta);
    std::fflush(stderr);

    if (finished) std::fprintf(stderr, "\n");
  };
}

/**
 * @brief Renderer entry point.
 * @return Exit code (0 on success, 1 on usage error or fatal exception).
 */
int main(int argc, char **argv) {
  std::string filepath, output_path = "output.exr", integrator_name = "path";
  int num_threads = 0;  // 0 => use hardware_concurrency()
  int spp_override = 0; // 0 => use the scene's sampleCount
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if      (arg == "-o" && i + 1 < argc) output_path     = argv[++i];
    else if (arg == "-i" && i + 1 < argc) integrator_name = argv[++i];
    else if ((arg == "-s" || arg == "--spp") && i + 1 < argc)
                                          spp_override    = std::atoi(argv[++i]);
    else if ((arg == "-t" || arg == "--threads") && i + 1 < argc)
                                          num_threads     = std::atoi(argv[++i]);
    else if (filepath.empty())            filepath        = arg;
  }

  Logger *logger = Logger::get_instance();
  logger->set_log_level(LogLevel::INFO);

  if (filepath.empty()) {
    std::cerr << "usage: kestrel <scene.xml> [-o output] "
                 "[-i path|direct|direct_bsdf] [-s spp] [-t threads]\n";
    return 1;
  }

  try {
    LOG_INFO("Kestrel ray tracer starting");

    std::unique_ptr<Scene> scene = read_from_file(filepath);
    int width = scene->camera().width();
    int height = scene->camera().height();

    LOG_INFO("Building BVH acceleration structure...");
    auto t_build0 = std::chrono::steady_clock::now();
    scene->build();
    auto t_build1 = std::chrono::steady_clock::now();
    double build_ms =
        std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();
    LOG_INFO("BVH construction complete (" + std::to_string(build_ms) + " ms)");

    std::vector<Color> pixels(width * height);

    std::unique_ptr<Integrator> integrator;
    if (integrator_name == "direct") {
      integrator = std::make_unique<DirectIntegrator>(DirectIntegrator::Strategy::Light);
      LOG_INFO("Integrator: direct illumination (light sampling)");
    } else if (integrator_name == "direct_bsdf") {
      integrator = std::make_unique<DirectIntegrator>(DirectIntegrator::Strategy::Bsdf);
      LOG_INFO("Integrator: direct illumination (BSDF sampling)");
    } else {
      integrator = std::make_unique<PathIntegrator>();
      LOG_INFO("Integrator: path tracer");
    }

    int spp = spp_override > 0
                  ? spp_override
                  : (scene->sample_count() > 0 ? scene->sample_count() : 1);
    if (num_threads <= 0)
      num_threads = static_cast<int>(std::thread::hardware_concurrency());

    LOG_INFO("Rendering " + std::to_string(width) + "x" +
             std::to_string(height) + " image...");
    auto t_render0 = std::chrono::steady_clock::now();
    integrator->render(*scene, scene->camera(), spp, pixels, num_threads,
                       make_progress_bar());
    auto t_render1 = std::chrono::steady_clock::now();
    double render_ms =
        std::chrono::duration<double, std::milli>(t_render1 - t_render0).count();
    LOG_INFO("Render complete (" + std::to_string(render_ms) + " ms)");
    write_image(output_path, pixels, width, height);
  } catch (const std::exception &e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
  return 0;
}