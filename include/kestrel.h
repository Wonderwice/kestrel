/**
 * @file kestrel.h
 * @brief Core structures for Kestrel ray tracer
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "material.h"
#include "pcg32.h"
#include "ray.h"
#include "vec3.h"
#include <memory>

namespace kestrel {

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
  Point3 point;             ///< 3D point of intersection
  Vec3 normal;              ///< Shading normal at the hit (oriented against the ray)
  Vec3 geo_normal;          ///< Geometric (face) normal, oriented against the ray. Used to
                            ///< offset shadow/secondary ray origins, avoiding the self-
                            ///< intersection acne the shading normal causes on smooth meshes.
  float t = 0.0f;           ///< Ray parameter at intersection (distance along ray)
  float u = 0.0f;           ///< Texture U coordinate at the hit point
  float v = 0.0f;           ///< Texture V coordinate at the hit point
  Vec3 tangent;             ///< Surface tangent (dP/du), used for normal mapping (triangles only)
  bool front_face = false;  ///< True if the ray hit the front face of the surface
  const Material *material = nullptr; ///< Material at the hit point

  HitRecord() = default;

  /**
   * @brief Set the surface normal and determine which face was hit
   * @param ray The ray that hit the surface
   * @param outward_normal The geometric outward-pointing normal
   *
   * This method ensures the normal always points against the ray direction,
   * which is useful for shading. It sets front_face to indicate whether
   * the ray came from outside (true) or inside (false) the surface.
   */
  void set_face_normal(const Ray &ray, const Vec3 &outward_normal) {
    front_face = Vec3::dot(ray.direction, outward_normal) < 0;
    normal = front_face ? outward_normal : -1.0f * outward_normal;
    // Sphere/Rectangle have no separate shading normal, so the geometric normal
    // coincides with the (ray-oriented) shading normal.
    geo_normal = normal;
  }
};

/**
 * @brief Parse a scene described in the Mitsuba format.
 * @param filepath Path to the scene file.
 * @return The parsed scene (owns all geometry, materials, textures and camera).
 */
std::unique_ptr<Scene> read_from_file(const std::string &filepath);

}  // namespace kestrel
