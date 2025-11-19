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

#include "camera.h"
#include "lambertian.h"
#include "light.h"
#include "ray.h"
#include "scene.h"
#include "sphere.h"
#include "vec3.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>

// PCG32 Random Number Generator
// Based on the PCG family by Melissa O'Neill
class PCG32 {
public:
  PCG32(uint64_t seed = 0x853c49e6748fea9bULL, uint64_t stream = 0xa02bdbf7bb3c0a7ULL)
      : state(0), inc((stream << 1u) | 1u) {
    next();
    state += seed;
    next();
  }

  uint32_t next() {
    uint64_t oldstate = state;
    state = oldstate * 6364136223846793005ULL + inc;
    uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }

  // Generate a random float in [0, 1)
  float next_float() {
    return static_cast<float>(next()) / static_cast<float>(0x100000000ULL);
  }

private:
  uint64_t state;
  uint64_t inc;
};

/**
 * @brief Determine pixel color by tracing a ray through the scene
 * @param ray The ray to trace
 * @param scene The scene to test for intersection
 * @param depth Current recursion depth
 * @return RGB color for this ray
 *
 * If the ray hits the sphere, returns a color based on the surface normal.
 * Otherwise, returns a blue-to-white gradient for the background sky.
 */
Color ray_color(const Ray &ray, const Scene &scene,
                int depth = 10) {
  // Limit recursion depth to prevent stack overflow
  if (depth <= 0) {
    return Color(0, 0, 0); // Return black if max depth reached
  }

  HitRecord rec;
  if (scene.hit(ray, 0.001f, 1000.0f, rec)) {

    // Add ambient lighting to prevent completely black shadows
    Color ambient_light = rec.material->get_color() * 0.1f;

    for (const auto &scene_light : scene.lights) {
      // Soft shadow sampling - take multiple samples across light surface
      const int shadow_samples = 4; // Increase for softer shadows (but slower)
      float shadow_factor = 0.0f;
      
      for (int i = 0; i < shadow_samples; ++i) {
        Vec3 light_sample_pos = scene_light.position;
        Vec3 light_dir = (light_sample_pos - rec.point).normalized();
        float light_distance = (light_sample_pos - rec.point).length();
        
        // Check for shadows
        Vec3 shadow_origin = rec.point + rec.normal * 0.001f;
        Ray shadow_ray(shadow_origin, light_dir);
        HitRecord shadow_rec;
        
        if (!scene.hit(shadow_ray, 0.001f, light_distance - 0.001f, shadow_rec)) {
          shadow_factor += 1.0f; // This sample is not in shadow
        }
      }
      
      shadow_factor /= static_cast<float>(shadow_samples);
      
      // Calculate lighting only for non-shadowed portion
      Vec3 light_dir = (scene_light.position - rec.point).normalized();
      float cos_theta = fmax(0.0f, Vec3::dot(rec.normal, light_dir));

      // Calculate direct lighting with proper distance falloff
      float distance = (scene_light.position - rec.point).length();
      Color direct_lighting = rec.material->get_color() * cos_theta *
                              scene_light.get_intensity() * shadow_factor /
                              distance;

      ambient_light += direct_lighting;
    }
    return ambient_light;
  }
  return Color(0.5f, 0.5f, 0.5f);
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
 * @return Exit code (0 = success)
 *
 * Sets up the scene, camera, and image buffer, then renders by tracing
 * one ray per pixel and writing the output to a PPM file.
 */
int main() {
  // Image settings
  const float aspect_ratio = 16.0f / 9.0f;
  const int image_width = 600;
  const int image_height = static_cast<int>(image_width / aspect_ratio);
  const int samples_per_pixel = 10;

  // Allocate pixel buffer
  std::vector<Color> pixels(image_width * image_height);

  // Camera setup: positioned at origin, looking down -Z axis
  Camera camera(Point3(0, 0, 0),  // look from: camera position
                Point3(0, 0, -1), // look at: point we're looking at
                Vec3(0, 1, 0),    // vup: "up" direction
                45.0f,            // vfov: vertical field of view in degrees
                aspect_ratio);

  const Lambertian lambertian(Color(0.75f, 0.25f, 0.25f));  // albedo
  const Lambertian lambertian2(Color(0.25f, 0.25f, 0.75f)); // albedo
 
  Sphere sphere1(Point3(0.0,  0.0, 0.0), 100.0f,lambertian);
  Sphere sphere2(Point3(-0.35, 0.35, -3.5), 0.25f, lambertian);
  Sphere sphere3(Point3(0.35, 0.35, -2.5), 0.35f, lambertian);
  Sphere sphere4(Point3(0.35, -0.35, -2.0), 0.3f, lambertian);
  Sphere sphere5(Point3(-0.35, -0.35, -4.0), 0.325f, lambertian);
  Sphere sphere6(Point3(-1.5,  0.0, -3.0), 0.5f, lambertian);
  Sphere sphere7(Point3(1.5,  0.0, -3.0), 0.5f, lambertian);
  Sphere sphere8(Point3(10.0,  0.0, -3.0), 0.5f, lambertian);
  Sphere sphere9(Point3(-10.0,  0.0, -3.0), 0.5f, lambertian);

  Light light2(Vec3(0,0,0), Vec3(1,1,1));
  Light light3(Vec3(-0.4,0.5,-3.0), Vec3(0.5,0.5,0.5));
  Light light4(Vec3(0,0,90), Vec3(1,1,1));

  Scene scene(camera);
  scene.add_object(sphere1);
  scene.add_object(sphere2);
  scene.add_object(sphere3);
  scene.add_object(sphere4);
  scene.add_object(sphere5);
  scene.add_object(sphere6);
  scene.add_object(sphere7);
  scene.add_object(sphere8);
  scene.add_object(sphere9);
  scene.add_lights(light2);
  scene.add_lights(light3);
  scene.add_lights(light4);

  // Initialize PCG random number generator
  PCG32 rng;

  // Render loop
  std::cout << "Rendering " << image_width << "x" << image_height
            << " image...\n";

  for (int j = 0; j < image_height; ++j) {
    // Progress indicator
    if (j % 50 == 0) {
      std::cout << "Scanline " << j << "/" << image_height << "\n";
    }

    for (int i = 0; i < image_width; ++i) {
      for (int s = 0; s < samples_per_pixel; ++s) {
        // Anti-aliasing: random offset within pixel
        float u = (i + rng.next_float()) / (image_width - 1);
        float v = (j + rng.next_float()) / (image_height - 1);

        // Generate ray through this pixel
        Ray ray = camera.get_ray(u, v);
        Color sample_color = ray_color(ray, scene);
        pixels[j * image_width + i] += sample_color;
      }
      // Average samples and store final pixel color
      pixels[j * image_width + i] *=
          1.0f / static_cast<float>(samples_per_pixel);
    }
  }

  // Write output file
  write_ppm("output.ppm", pixels, image_width, image_height);
  std::cout << "Done! Output written to output.ppm\n";

  return 0;
}