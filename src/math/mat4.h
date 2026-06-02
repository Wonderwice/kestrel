/**
 * @file mat4.h
 * @brief 4x4 row-major affine transform used for scene geometry.
 * @author Alexei Czornyj
 * @date 2026
 *
 * Single source of truth for the matrix math that used to be duplicated between
 * parser.cpp (mat4_* free functions) and each shape's transform(float[16]).
 * Stores 16 floats in row-major order; translation lives in m[3], m[7], m[11].
 */

#pragma once

#include "constants.h"
#include "vec3.h"
#include <cmath>

namespace kestrel {

class Mat4 {
public:
  float m[16];

  /// Identity matrix.
  static Mat4 identity() {
    Mat4 r;
    for (int i = 0; i < 16; ++i) r.m[i] = 0.f;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.f;
    return r;
  }

  /// Build from 16 row-major floats (e.g. a Mitsuba <matrix value="...">).
  static Mat4 from_row_major(const float v[16]) {
    Mat4 r;
    for (int i = 0; i < 16; ++i) r.m[i] = v[i];
    return r;
  }

  static Mat4 scaling(const Vec3 &s) {
    Mat4 r = identity();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
    return r;
  }

  static Mat4 translation(const Vec3 &t) {
    Mat4 r = identity();
    r.m[3] = t.x; r.m[7] = t.y; r.m[11] = t.z;
    return r;
  }

  /// Axis-angle rotation (Rodrigues), angle in degrees.
  static Mat4 rotation(float angle_deg, const Vec3 &axis_in) {
    Vec3 a = axis_in.normalized();
    float r = angle_deg * DEG_TO_RAD;
    float c = std::cos(r), s = std::sin(r), t = 1.f - c;
    float x = a.x, y = a.y, z = a.z;
    Mat4 out = identity();
    out.m[0] = c + x * x * t;     out.m[1] = x * y * t - z * s; out.m[2]  = x * z * t + y * s;
    out.m[4] = y * x * t + z * s; out.m[5] = c + y * y * t;     out.m[6]  = y * z * t - x * s;
    out.m[8] = z * x * t - y * s; out.m[9] = z * y * t + x * s; out.m[10] = c + z * z * t;
    return out;
  }

  /// Matrix product: (*this) * rhs (row-major).
  Mat4 operator*(const Mat4 &rhs) const {
    Mat4 out;
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c) {
        float s = 0.f;
        for (int k = 0; k < 4; ++k) s += m[r * 4 + k] * rhs.m[k * 4 + c];
        out.m[r * 4 + c] = s;
      }
    return out;
  }

  /// Transform a position (applies translation).
  Point3 transform_point(const Point3 &p) const {
    return Point3(m[0] * p.x + m[1] * p.y + m[2]  * p.z + m[3],
                  m[4] * p.x + m[5] * p.y + m[6]  * p.z + m[7],
                  m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]);
  }

  /// Transform a direction (ignores translation).
  Vec3 transform_vector(const Vec3 &v) const {
    return Vec3(m[0] * v.x + m[1] * v.y + m[2]  * v.z,
                m[4] * v.x + m[5] * v.y + m[6]  * v.z,
                m[8] * v.x + m[9] * v.y + m[10] * v.z);
  }

  /// Transform a normal by the inverse-transpose of the upper-left 3x3,
  /// returned normalized. Computed via cofactors to avoid a full inverse.
  Vec3 transform_normal(const Vec3 &n) const {
    float c0x = m[5]*m[10] - m[6]*m[9];
    float c0y = m[6]*m[8]  - m[4]*m[10];
    float c0z = m[4]*m[9]  - m[5]*m[8];
    float c1x = m[2]*m[9]  - m[1]*m[10];
    float c1y = m[0]*m[10] - m[2]*m[8];
    float c1z = m[1]*m[8]  - m[0]*m[9];
    float c2x = m[1]*m[6]  - m[2]*m[5];
    float c2y = m[2]*m[4]  - m[0]*m[6];
    float c2z = m[0]*m[5]  - m[1]*m[4];
    // c0/c1/c2 are the ROWS of the cofactor matrix (= (M^-1)^T up to det). Apply
    // them as rows: result_i = cofactor_row_i · n. Using them as columns would
    // transpose the matrix, which for a rotation applies the INVERSE rotation and
    // rotates transformed normals the wrong way (broke rotated meshes' shading).
    return Vec3(c0x*n.x + c0y*n.y + c0z*n.z,
                c1x*n.x + c1y*n.y + c1z*n.z,
                c2x*n.x + c2y*n.y + c2z*n.z).normalized();
  }
};

}  // namespace kestrel
