/**
 * @file lambertian.h
 * @brief Lambertian diffuse reflection model for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "lambertian.h"
#include "ray.h"
#include "vec3.h"

// Forward declaration
class Lambertian;

/**
 * @struct HitRecord
 * @brief Information about a ray-geometry intersection
 *
 * Contains all relevant information about where and how a ray intersected
 * a surface, including position, normal, and parameter t.
 */
struct HitRecord {
  Point3 point; ///< 3D point of intersection
  Vec3 normal;  ///< Surface normal at intersection (always points outward from
                ///< surface)
  float t;      ///< Ray parameter at intersection (distance along ray)
  bool front_face;           ///< True if ray hit the front face of the surface
  const Lambertian *material; ///< Material at the hit point

  /**
   * @brief Construct a new Hit Record object
   */
  HOST_DEVICE HitRecord() : material(nullptr) {}

  /**
   * @brief Set the surface normal and determine which face was hit
   * @param ray The ray that hit the surface
   * @param outward_normal The geometric outward-pointing normal
   *
   * This method ensures the normal always points against the ray direction,
   * which is useful for shading. It sets front_face to indicate whether
   * the ray came from outside (true) or inside (false) the surface.
   */
  HOST_DEVICE void set_face_normal(const Ray &ray, const Vec3 &outward_normal) {
    front_face = Vec3::dot(ray.direction, outward_normal) < 0;
    normal = front_face ? outward_normal : -1.0f * outward_normal;
  }
};

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
  HOST_DEVICE Lambertian(Color albedo) : albedo(albedo) {}

  HOST_DEVICE bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered) const {
    Vec3 scatter_direction = rec.normal + Vec3::random_unit_vector();
    scattered = Ray(rec.point, scatter_direction);
    attenuation = albedo;
    return true;
  }

  HOST_DEVICE Color get_color() const { return albedo; }

private:
  Color albedo;
};

#endif