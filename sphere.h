/**
 * @file sphere.h
 * @brief Sphere primitive with ray intersection
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef SPHERE_H
#define SPHERE_H

#include "ray.h"
#include "vec3.h"
#include "lambertian.h"

/**
 * @class Sphere
 * @brief Sphere primitive defined by center and radius
 *
 * Implements analytical ray-sphere intersection using the quadratic formula.
 * The sphere is defined implicitly as all points p where |p - center|² =
 * radius².
 */
class Sphere {
public:
  Point3 center;       ///< Center of the sphere in world space
  float radius;        ///< Radius of the sphere
  const Lambertian material; ///< Material of the sphere

  /**
   * @brief Construct sphere from center and radius
   * @param center Center point of the sphere
   * @param radius Radius of the sphere (must be positive)
   * @param material Material of the sphere
   */
  HOST_DEVICE Sphere(Point3 center, float radius, const Lambertian &material)
      : center(center), radius(radius), material(material) {}

  /**
   * @brief Test ray-sphere intersection
   * @param ray The ray to test for intersection
   * @param t_min Minimum valid t parameter (near clipping)
   * @param t_max Maximum valid t parameter (far clipping)
   * @param rec Output parameter filled with intersection details if hit occurs
   * @return True if intersection exists in range [t_min, t_max], false
   * otherwise
   *
   * Uses the quadratic formula to solve for ray-sphere intersection.
   * The ray is given by P(t) = origin + t * direction, and we solve:
   * |P(t) - center|² = radius²
   *
   * This expands to: at² + 2bt + c = 0, where:
   * - a = direction · direction
   * - b = (origin - center) · direction
   * - c = |origin - center|² - radius²
   */
  HOST_DEVICE bool hit(const Ray &ray, float t_min, float t_max,
                       HitRecord &rec) const {
    // Solve quadratic equation for ray-sphere intersection
    Vec3 oc = ray.origin - center;
    float a = ray.direction.length_squared();
    float half_b = Vec3::dot(oc, ray.direction);
    float c = oc.length_squared() - radius * radius;

    // Check discriminant to determine if intersection exists
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0)
      return false;

    float sqrtd = std::sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range [t_min, t_max]
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || t_max < root) {
      // Try the other root
      root = (-half_b + sqrtd) / a;
      if (root < t_min || t_max < root)
        return false;
    }

    // Fill hit record with intersection information
    rec.t = root;
    rec.point = ray.at(rec.t);
    Vec3 outward_normal = (rec.point - center) / radius;
    rec.set_face_normal(ray, outward_normal);

    return true;
  }
};

#endif