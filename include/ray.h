/**
 * @file ray.h
 * @brief Ray representation for ray tracing
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "vec3.h"

/**
 * @class Ray
 * @brief Represents a ray with origin and direction
 *
 * A ray is defined by the parametric equation: P(t) = origin + t * direction
 * where t >= 0. Used for all ray-geometry intersection tests.
 */
namespace kestrel {

class Ray {
public:
  Point3 origin;  ///< Ray origin point
  Vec3 direction; ///< Ray direction (not necessarily normalized)
  Vec3 inv_direction; ///< Precomputed 1/direction for fast AABB tests

  /**
   * @brief Default constructor
   */
  Ray() {}

  /**
   * @brief Construct ray from origin and direction
   * @param origin Starting point of the ray
   * @param direction Direction vector of the ray
   */
  Ray(const Point3 &origin_, const Vec3 &direction_)
      : origin(origin_), direction(direction_),
        inv_direction(1.0f / direction_.x, 1.0f / direction_.y,
                      1.0f / direction_.z) {}

  /**
   * @brief Evaluate ray at parameter t
   * @param t Parameter value (distance along ray)
   * @return Point on the ray at distance t: origin + t * direction
   */
  Point3 at(float t) const { return origin + t * direction; }
};

}  // namespace kestrel
