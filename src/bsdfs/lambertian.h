/**
 * @file lambertian.h
 * @brief Lambertian diffuse reflection model for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "kestrel.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class Lambertian
 * @brief Implements Lambertian diffuse reflection
 *
 * This class provides methods to compute diffuse reflection directions
 * based on the Lambertian reflection model. It generates random
 * directions over the hemisphere oriented around the surface normal.
 */
class Lambertian : public Material {
public:
  /**
   * @brief Default constructor
   */
  HOST_DEVICE Lambertian() {}
  /**
   * @brief Generate a random diffuse reflection direction
   * @param normal Surface normal at the hit point
   * @return Random direction over hemisphere oriented by normal
   *
   * Uses cosine-weighted hemisphere sampling to favor directions
   * closer to the normal, simulating Lambertian reflection.
   */
  HOST_DEVICE Lambertian(Color albedo) : albedo(albedo), Material(0.0f) {}

  /**
   * @brief Generate a diffuse scatter direction
   * @param incoming Incoming ray direction
   * @param rec Hit record with surface normal
   * @param attenuation Output color attenuation
   * @param scattered Output scattered ray
   * @return True if scattering occurred
   */
  HOST_DEVICE bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered) const {
    Vec3 scatter_direction = rec.normal + Vec3::random_unit_vector();
    scattered = Ray(rec.point, scatter_direction);
    attenuation =
        albedo /
        3.14159265358979323846f; // Normalize by pi for energy conservation
    return true;
  }

  /**
   * @brief Get the color of the Lambertian material
   * @return Albedo color normalized by pi
   */
  HOST_DEVICE Color get_color() const {
    return albedo / 3.14159265358979323846f;
  }

private:
  Color albedo;
};

#endif