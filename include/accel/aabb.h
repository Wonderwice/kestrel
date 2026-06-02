/**
 * @file aabb.h
 * @brief Axis-aligned bounding box with ray-slab intersection.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "ray.h"
#include "vec3.h"
#include <limits>

namespace kestrel {

struct AABB {
    Point3 min;
    Point3 max;

    AABB()
        : min(Point3(std::numeric_limits<float>::infinity(),
                     std::numeric_limits<float>::infinity(),
                     std::numeric_limits<float>::infinity())),
          max(Point3(-std::numeric_limits<float>::infinity(),
                     -std::numeric_limits<float>::infinity(),
                     -std::numeric_limits<float>::infinity())) {}

    /// Grow the box to include a point.
    void expand(const Point3 &p) {
        if (min.x > p.x) min.x = p.x;
        if (min.y > p.y) min.y = p.y;
        if (min.z > p.z) min.z = p.z;
        if (max.x < p.x) max.x = p.x;
        if (max.y < p.y) max.y = p.y;
        if (max.z < p.z) max.z = p.z;
    }

    /// Grow the box to include another box.
    void expand(const AABB &other) {
        if (min.x > other.min.x) min.x = other.min.x;
        if (min.y > other.min.y) min.y = other.min.y;
        if (min.z > other.min.z) min.z = other.min.z;
        if (max.x < other.max.x) max.x = other.max.x;
        if (max.y < other.max.y) max.y = other.max.y;
        if (max.z < other.max.z) max.z = other.max.z;
    }

    /// Grow any near-zero-extent axis so the box is never degenerate. Planar
    /// geometry (e.g. a floor of coplanar triangles) otherwise yields a
    /// zero-thickness box that the slab test rejects for rays meeting its plane.
    void pad() {
        float e = std::max(1e-5f, (max - min).length() * 1e-4f);
        for (int a = 0; a < 3; ++a) {
            if (max[a] - min[a] < e) { min[a] -= 0.5f * e; max[a] += 0.5f * e; }
        }
    }

    /// Longest axis of the box (0=x, 1=y, 2=z).
    int longest_axis() const {
        float x = max.x - min.x, y = max.y - min.y, z = max.z - min.z;
        if (x >= y && x >= z) return 0;
        if (y >= z) return 1;
        return 2;
    }

    /// Slab test against a ray over [tMin, tMax]. The per-axis early-out matters:
    /// most tests during BVH traversal are rejections, and bailing after the
    /// first failing axis beats a branchless form that always touches all three
    /// (measured: branchless was ~20-30% slower on intersection-heavy scenes).
    bool intersect(const Ray &ray, float tMin, float tMax) const {
        for (size_t axis = 0; axis < 3; axis++) {
            float invD = ray.inv_direction[axis];
            float t0 = (min[axis] - ray.origin[axis]) * invD;
            float t1 = (max[axis] - ray.origin[axis]) * invD;
            if (invD < 0.0f) std::swap(t0, t1);
            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;
            if (tMax <= tMin) return false;
        }
        return true;
    }

    /// Slab test that also reports the entry distance `tEntry` (the near-plane
    /// crossing, clamped to tMin). Used to visit the nearer BVH child first so the
    /// running closest-hit distance can cull the farther subtree.
    bool intersect(const Ray &ray, float tMin, float tMax, float &tEntry) const {
        float entry = tMin;
        for (size_t axis = 0; axis < 3; axis++) {
            float invD = ray.inv_direction[axis];
            float t0 = (min[axis] - ray.origin[axis]) * invD;
            float t1 = (max[axis] - ray.origin[axis]) * invD;
            if (invD < 0.0f) std::swap(t0, t1);
            entry = t0 > entry ? t0 : entry;
            tMax  = t1 < tMax ? t1 : tMax;
            if (tMax <= entry) return false;
        }
        tEntry = entry;
        return true;
    }

    /// Surface area of the box (0 for an empty/degenerate box). Used by the SAH.
    float surface_area() const {
        float dx = max.x - min.x, dy = max.y - min.y, dz = max.z - min.z;
        if (dx < 0.0f || dy < 0.0f || dz < 0.0f) return 0.0f;
        return 2.0f * (dx * dy + dy * dz + dz * dx);
    }
};

}  // namespace kestrel
