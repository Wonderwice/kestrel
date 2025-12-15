/**
 * @file main.cpp
 * @brief Main renderer entry point
 * @author Alexei Czornyj
 * @date 2025
 *
 * This is the main rendering loop. It generates rays
 * through each pixel of the image, tests for intersection with a sphere,
 * and colors pixels based on surface normals. Output is written to PPM format.
 */

#include "kestrel.h"
#include "bsdfs/conductor.h"
#include "bsdfs/lambertian.h"
#include "camera.h"
#include "light.h"
#include "logger.h"
#include "plymesh.h"
#include "ray.h"
#include "scene.h"
#include "sphere.h"
#include "triangle.h"
#include "vec3.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

/**
 * @brief Determine pixel color by tracing a ray through the scene
 * @param ray The ray to trace
 * @param scene The scene to test for intersection
 * @param depth Current recursion depth
 * @return RGB color for this ray
 *
 * If the ray hits the sphere, returns a color based on the surface normal.
 */
Color ray_color(const Ray &ray, const Scene &scene, int depth = 10) {
  if (depth <= 0) {
    return Color(0, 0, 0); // Return black if max depth reached
  }

  HitRecord rec;
  if (scene.hit(ray, 0.001f, 1000.0f, rec)) {
    // Add ambient lighting to prevent completely black shadows
    Color final_color = Color(0, 0, 0);

    for (const auto &scene_light : scene.lights) {
      // Soft shadow sampling - take multiple samples across light surface
      const int shadow_samples = 2; // Increase for softer shadows (but slower)
      float shadow_factor = 0.0f;

      for (int i = 0; i < shadow_samples; ++i) {
        Vec3 light_dir = scene_light.sample_direction(rec.point);
        Vec3 light_sample_pos = scene_light.position;
        float light_distance = (light_sample_pos - rec.point).length();

        // Check for shadows
        Vec3 shadow_origin = rec.point + rec.normal * 0.001f;
        Ray shadow_ray(shadow_origin, light_dir);
        HitRecord shadow_rec;

        if (!scene.hit(shadow_ray, 0.001f, light_distance - 0.001f,
                       shadow_rec)) {
          shadow_factor += 1.0f; // This sample is not in shadow
        }
      }

      shadow_factor /= static_cast<float>(shadow_samples);

      // Calculate lighting only for non-shadowed portion
      Vec3 light_dir = (scene_light.position - rec.point).normalized();
      float cos_theta = fmax(0.0f, Vec3::dot(rec.normal, light_dir));

      // Calculate direct lighting with proper distance falloff
      float distance = (scene_light.position - rec.point).length();
      Color direct_lighting =
          rec.material->get_color() * cos_theta * scene_light.get_intensity() *
          shadow_factor /
          (distance * distance + 1e-4f); // Avoid division by zero

      final_color += direct_lighting;
    }

    // Handle reflections if material is reflective
    Color reflected_color(0, 0, 0);
    if (rec.material->reflectivity > 0.0f) {
      Vec3 incident_dir = ray.direction.normalized();
      Vec3 reflected_dir =
          incident_dir -
          2.0f * Vec3::dot(incident_dir, rec.normal) * rec.normal;
      Ray reflected_ray(rec.point + rec.normal * 0.001f, reflected_dir);
      reflected_color = ray_color(reflected_ray, scene, depth - 1) *
                        rec.material->reflectivity * rec.material->get_color();
    }

    // Blend direct lighting with reflections
    return final_color * (1.0f - rec.material->reflectivity) + reflected_color;
  }

  return Color(0.0f); // Background sky color
}

/**
 * @brief Render the scene into the pixel buffer using multithreading
 * @param scene The scene to render
 * @param camera The camera through which to render
 * @param image_width Width of the image in pixels
 * @param image_height Height of the image in pixels
 * @param samples_per_pixel Number of samples per pixel for anti-aliasing
 * @param pixels Output pixel buffer (size = image_width * image_height)
 * @param num_threads Number of threads to use for rendering
 *
 * This function divides the rendering workload across multiple threads,
 * each processing scanlines of the image until all rows are completed.
 */
void render_scene(const Scene &scene, const Camera &camera, int samples_per_pixel,
                  std::vector<Color> &pixels, int num_threads) {
  // Render loop
  LOG_INFO("Rendering " + std::to_string(camera.width()) + "x" +
           std::to_string(camera.height()) + " image...");
  // Parallelize with std::thread
  std::vector<std::thread> threads;
  std::atomic<int> next_row(0);
  // Initialize PCG random number generator
  PCG32 rng;
  auto render_worker = [&](int thread_id) {
    PCG32 thread_rng(rng.next() + thread_id); // Per-thread RNG
    while (true) {
      int j = next_row.fetch_add(1);
      if (j >= camera.height())
        break;

      if (j % 50 == 0) {
        LOG_INFO("Scanline " + std::to_string(j) + "/" + std::to_string(camera.height()));
      }

      for (int i = 0; i < camera.width(); ++i) {
        Color pixel_color(0, 0, 0);
        for (int s = 0; s < samples_per_pixel; ++s) {
          float u = (i + thread_rng.next_float()) / (camera.width() - 1);
          float v = (j + thread_rng.next_float()) / (camera.height() - 1);
          Ray ray = camera.get_ray(u, v);
          pixel_color += ray_color(ray, scene);
        }
        pixel_color *= 1.0f / static_cast<float>(samples_per_pixel);
        pixels[j * camera.width() + i] = pixel_color;
      }
    }
  };

  // Launch threads
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(render_worker, t);
  }
  for (auto &th : threads) {
    th.join();
  }
}

/**
 * @brief Write image data to PPM file format
 * @param filename Output filename (should end in .ppm)
 * @param pixels Array of RGB colors, size width * height
 * @param width Image width in pixels
 * @param height Image height in pixels
 *
 * PPM is a simple uncompressed image format. The output can be viewed
 * with many image viewers or converted to other formats with ImageMagick.
 */
void write_ppm(const std::string &filename, const std::vector<Color> &pixels,
               int width, int height) {
  std::ofstream file(filename);

  // PPM header: P3 = ASCII color, width height, max_color_value
  file << "P3\n" << width << " " << height << "\n255\n";

  // Write pixels from top to bottom (PPM convention)
  for (int j = height - 1; j >= 0; --j) {
    for (int i = 0; i < width; ++i) {
      Color pixel = pixels[j * width + i];

      // Clamp values to [0, 1] and convert to [0, 255]
      float gamma = 1.0f / 2.2f;
      int ir = static_cast<int>(
          255.99f * pow(std::min(std::max(pixel.x, 0.0f), 1.0f), gamma));
      int ig = static_cast<int>(
          255.99f * pow(std::min(std::max(pixel.y, 0.0f), 1.0f), gamma));
      int ib = static_cast<int>(
          255.99f * pow(std::min(std::max(pixel.z, 0.0f), 1.0f), gamma));

      file << ir << " " << ig << " " << ib << "\n";
    }
  }

  file.close();
}

/**
 * @brief Main rendering function
 * @return Exit code
 *
 * Sets up the scene, camera, and image buffer, then renders by tracing
 * one ray per pixel and writing the output to a PPM file.
 */
int main(int argc, char **argv) {
  // Initialize logger
  Logger *logger = Logger::get_instance();
  logger->set_log_level(LogLevel::INFO);
  LOG_INFO("Kestrel ray tracer starting");

  Scene *scene = read_from_file("data/scene.xml");
  int width = scene->camera->width();
  int height = scene->camera->height();

  std::vector<Color> pixels(width * height);

  render_scene(*scene, *(scene->camera), 1, pixels, 12);
  delete scene;
  write_ppm("output.ppm", pixels, width, height);
  return 0;
}