/**
 * @file lambertian.h
 * @brief Lambertian diffuse reflection model for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "camera.h"
#include "ray.h"
#include "sphere.h"
#include "vec3.h"

/**
 * @class Lambertian
 * @brief Implements Lambertian diffuse reflection
 *
 * This class provides methods to compute diffuse reflection directions
 * based on the Lambertian reflection model. It generates random
 * directions over the hemisphere oriented around the surface normal.
 */
class Lambertian {
public:
  /**
   * @brief Generate a random diffuse reflection direction
   * @param normal Surface normal at the hit point
   * @return Random direction over hemisphere oriented by normal
   *
   * Uses cosine-weighted hemisphere sampling to favor directions
   * closer to the normal, simulating Lambertian reflection.
   */
  HOST_DEVICE Lambertian(Color albedo) : albedo(albedo) {}

  HOST_DEVICE bool scatter(const Ray &incoming, const HitRecord &rec, Color& attenuation, Ray& scattered) const {
    Vec3 scatter_direction = rec.normal + Vec3::random_unit_vector();
    scattered = Ray(rec.point, scatter_direction);
    attenuation = albedo;
    return true;
  }

private:
  Color albedo;
};

#endif