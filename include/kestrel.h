/**
 * @file kestrel.h
 * @brief Core structures for Kestrel ray tracer
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef KESTREL_H
#define KESTREL_H

#include "bsdfs/material.h"
#include "pcg32.h"
#include "ray.h"
#include "vec3.h"

class Material; // Forward declaration
class Scene;

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
  bool front_face;          ///< True if ray hit the front face of the surface
  const Material *material; ///< Material at the hit point

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
 * @brief Parse a scene described in the Mitsuba format.
 * @param filepath Path to the scene file.
 */
Scene *read_from_file(const std::string &filepath);

#endif // KESTREL_H