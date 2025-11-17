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

#include <iostream>
#include <fstream>
#include <vector>
#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "camera.h"

/**
 * @brief Determine pixel color by tracing a ray through the scene
 * @param ray The ray to trace
 * @param sphere The sphere to test for intersection
 * @return RGB color for this ray
 * 
 * If the ray hits the sphere, returns a color based on the surface normal.
 * Otherwise, returns a blue-to-white gradient for the background sky.
 */
Color ray_color(const Ray& ray, const Sphere& sphere) {
    HitRecord rec;
    if (sphere.hit(ray, 0.0f, 1000.0f, rec)) {
        // Color based on normal direction (maps -1..1 to 0..1)
        // Red = X, Green = Y, Blue = Z
        return 0.5f * (rec.normal + Vec3(1, 1, 1));
    }
    
    // Background: blue-to-white vertical gradient
    Vec3 unit_direction = ray.direction.normalized();
    float t = 0.5f * (unit_direction.y + 1.0f);
    return (1.0f - t) * Color(1.0f, 1.0f, 1.0f) + t * Color(0.5f, 0.7f, 1.0f);
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
void write_ppm(const std::string& filename, const std::vector<Color>& pixels, int width, int height) {
    std::ofstream file(filename);
    
    // PPM header: P3 = ASCII color, width height, max_color_value
    file << "P3\n" << width << " " << height << "\n255\n";
    
    // Write pixels from top to bottom (PPM convention)
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            Color pixel = pixels[j * width + i];
            
            // Clamp values to [0, 1] and convert to [0, 255]
            int ir = static_cast<int>(255.99f * std::min(std::max(pixel.x, 0.0f), 1.0f));
            int ig = static_cast<int>(255.99f * std::min(std::max(pixel.y, 0.0f), 1.0f));
            int ib = static_cast<int>(255.99f * std::min(std::max(pixel.z, 0.0f), 1.0f));
            
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
    const int image_width = 1920;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    
    // Allocate pixel buffer
    std::vector<Color> pixels(image_width * image_height);
    
    // Camera setup: positioned at origin, looking down -Z axis
    Camera camera(
        Point3(0, 0, 0),      // look from: camera position
        Point3(0, 0, -1),     // look at: point we're looking at
        Vec3(0, 1, 0),        // vup: "up" direction
        90.0f,                // vfov: vertical field of view in degrees
        aspect_ratio
    );
    
    // Scene: single sphere centered at (0, 0, -1) with radius 0.5
    Sphere sphere(Point3(0, 0, -1), 0.5f);
    
    // Render loop
    std::cout << "Rendering " << image_width << "x" << image_height << " image...\n";
    
    for (int j = 0; j < image_height; ++j) {
        // Progress indicator
        if (j % 50 == 0) {
            std::cout << "Scanline " << j << "/" << image_height << "\n";
        }
        
        for (int i = 0; i < image_width; ++i) {
            // Calculate normalized pixel coordinates [0, 1]
            float u = float(i) / (image_width - 1);
            float v = float(j) / (image_height - 1);
            
            // Generate ray through this pixel
            Ray ray = camera.get_ray(u, v);
            Color pixel_color = ray_color(ray, sphere);
            
            // Store in buffer (row-major order)
            pixels[j * image_width + i] = pixel_color;
        }
    }
    
    // Write output file
    write_ppm("output.ppm", pixels, image_width, image_height);
    std::cout << "Done! Output written to output.ppm\n";
    
    return 0;
}