/**
 * @file ray.h
 * @brief Ray representation for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef RAY_H
#define RAY_H

#include "vec3.h"

/**
 * @class Ray
 * @brief Represents a ray with origin and direction
 * 
 * A ray is defined by the parametric equation: P(t) = origin + t * direction
 * where t >= 0. Used for all ray-geometry intersection tests.
 */
class Ray {
public:
    Point3 origin;    ///< Ray origin point
    Vec3 direction;   ///< Ray direction (not necessarily normalized)

    /**
     * @brief Default constructor
     */
    HOST_DEVICE Ray() {}
    
    /**
     * @brief Construct ray from origin and direction
     * @param origin Starting point of the ray
     * @param direction Direction vector of the ray
     */
    HOST_DEVICE Ray(const Point3& origin, const Vec3& direction)
        : origin(origin), direction(direction) {}

    /**
     * @brief Evaluate ray at parameter t
     * @param t Parameter value (distance along ray)
     * @return Point on the ray at distance t: origin + t * direction
     */
    HOST_DEVICE Point3 at(float t) const {
        return origin + t * direction;
    }
};

#endif