/**
 * @file onb.h
 * @brief Orthonormal-basis helper shared by the glossy/microfacet BSDFs.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "vec3.h"
#include <cmath>

/// Build an orthonormal basis (tangent `t`, bitangent `b`) around a unit axis `n`.
namespace kestrel {

inline void build_onb(const Vec3 &n, Vec3 &t, Vec3 &b) {
  Vec3 up = (std::abs(n.y) < 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
  t = Vec3::cross(up, n).normalized();
  b = Vec3::cross(n, t);
}

}  // namespace kestrel
