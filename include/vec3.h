/**
 * @file vec3.h
 * @brief 3D vector mathematics for CPU and GPU execution
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "pcg32.h"
#include <algorithm>
#include <cmath>
#include <iostream>

/**
 * @class Vec3
 * @brief A 3D vector class for positions, directions, colors, and normals.
 *
 * Hot-path arithmetic is defined inline below the class so it inlines and
 * vectorizes into every caller (BVH traversal, Möller–Trumbore, BSDFs).
 */
namespace kestrel {

class Vec3 {
public:
  float x, y, z; ///< Vector components

  /**
   * @brief Default constructor - initializes to zero vector
   */
  Vec3();

  /**
   * @brief Construct vector with all components set to the same value
   * @param v Value for x, y, and z components
   */
  Vec3(float v);

  /**
   * @brief Construct vector from three components
   * @param x X component
   * @param y Y component
   * @param z Z component
   */
  Vec3(float x, float y, float z);
  /**
   * @brief Vector addition
   * @param v Vector to add
   * @return Sum of this vector and v
   */
  Vec3 operator+(const Vec3 &v) const;

  /**
   * @brief Vector subtraction
   * @param v Vector to subtract
   * @return Difference of this vector and v
   */
  Vec3 operator-(const Vec3 &v) const;

  /**
   * @brief Scalar multiplication
   * @param t Scalar value
   * @return This vector scaled by t
   */
  Vec3 operator*(float t) const;

  /**
   * @brief Component-wise multiplication
   * @param v Vector to multiply with
   * @return Component-wise product
   */
  Vec3 operator*(const Vec3 &v) const;

  /**
   * @brief Scalar division
   * @param t Scalar divisor
   * @return This vector divided by t
   */
  Vec3 operator/(float t) const;

  /**
   * @brief In-place vector addition
   * @param v Vector to add
   * @return Reference to this vector after addition
   */
  Vec3 &operator+=(const Vec3 &v);

  /**
   * @brief In-place scalar multiplication
   * @param t Scalar multiplier
   * @return Reference to this vector after scaling
   */
  Vec3 &operator*=(float t);

  /**
   * @brief In-place component-wise multiplication
   * @param v Vector to multiply with
   * @return Reference to this vector after scaling
   */
  Vec3 &operator*=(const Vec3 &v);

  /**
   * @brief In-place scalar division
   * @param t Scalar divisor
   * @return Reference to this vector after division
   */
  Vec3 &operator/=(float t);

  float operator[](size_t i) const;
  float &operator[](size_t i);

  /// Unary negation.
  Vec3 operator-() const;

  /**
   * @brief Calculate the length (magnitude) of the vector
   * @return Euclidean length of the vector
   */
  float length() const;

  /**
   * @brief Calculate the squared length of the vector
   * @return Squared Euclidean length (avoids sqrt for performance)
   */
  float length_squared() const;

  /**
   * @brief Get a unit-length version of this vector
   * @return Normalized vector with length 1
   */
  Vec3 normalized() const;

  /**
   * @brief Compute dot product of two vectors
   * @param a First vector
   * @param b Second vector
   * @return Scalar dot product a · b
   */
  static float dot(const Vec3 &a, const Vec3 &b);

  /**
   * @brief Compute cross product of two vectors
   * @param a First vector
   * @param b Second vector
   * @return Vector perpendicular to both a and b (a × b)
   */
  static Vec3 cross(const Vec3 &a, const Vec3 &b);

  /**
   * @brief Generate a random vector with each component in the range [min,
   * max].
   * @param min The minimum value that any component of the vector can take.
   * @param max The maximum value that any component of the vector can take.
   * @return A Vec3 object representing the random vector where each component
   *         is in the range [min, max].
   */
  static Vec3 random(float min, float max, PCG32 &rng);

  /**
   * @brief Generate a random unit vector.
   * @return A Vec3 object representing a random unit vector.
   */
  static Vec3 random_unit_vector(PCG32 &rng);
};

// ---------------------------------------------------------------------------
// Hot-path math is defined inline in the header so it inlines/vectorizes into
// every caller (BVH traversal, Möller–Trumbore, BSDFs). The out-of-line copies
// used to live in vec3.cpp and could not inline across translation units.
// ---------------------------------------------------------------------------

inline Vec3::Vec3() : x(0), y(0), z(0) {}
inline Vec3::Vec3(float v) : x(v), y(v), z(v) {}
inline Vec3::Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

inline Vec3 Vec3::operator+(const Vec3 &v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
inline Vec3 Vec3::operator-(const Vec3 &v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
inline Vec3 Vec3::operator*(float t) const { return Vec3(x * t, y * t, z * t); }
inline Vec3 Vec3::operator*(const Vec3 &v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
inline Vec3 Vec3::operator/(float t) const { return Vec3(x / t, y / t, z / t); }

inline Vec3 &Vec3::operator+=(const Vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
inline Vec3 &Vec3::operator*=(float t) { x *= t; y *= t; z *= t; return *this; }
inline Vec3 &Vec3::operator*=(const Vec3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
inline Vec3 &Vec3::operator/=(float t) { x /= t; y /= t; z /= t; return *this; }

inline float Vec3::operator[](size_t i) const { return (&x)[i]; }
inline float &Vec3::operator[](size_t i) { return (&x)[i]; }

inline Vec3 Vec3::operator-() const { return Vec3(-x, -y, -z); }

inline float Vec3::length() const { return std::sqrt(x * x + y * y + z * z); }
inline float Vec3::length_squared() const { return x * x + y * y + z * z; }

inline Vec3 Vec3::normalized() const {
  float len = length();
  if (len == 0.0f) return Vec3(0.0f, 0.0f, 0.0f);
  return Vec3(x / len, y / len, z / len);
}

inline float Vec3::dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Vec3::cross(const Vec3 &a, const Vec3 &b) {
  return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

/**
 * @brief Scalar multiplication with scalar on left side
 */
inline Vec3 operator*(float t, const Vec3 &v) { return Vec3(t * v.x, t * v.y, t * v.z); }

/// Component-wise minimum of two vectors.
inline Vec3 min(const Vec3 &a, const Vec3 &b) {
  return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

/// Component-wise maximum of two vectors.
inline Vec3 max(const Vec3 &a, const Vec3 &b) {
  return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

/**
 * @brief Reflect vector v around normal n
 */
inline Vec3 reflect(const Vec3 &v, const Vec3 &n) {
  return v - 2.0f * Vec3::dot(v, n) * n;
}

/**
 * @brief Refract vector uv through a surface with the given normal and IOR ratio.
 * Returns the zero vector on total internal reflection.
 */
inline Vec3 refract(const Vec3 &uv, const Vec3 &n, float etai_over_etat) {
  float cos_theta = std::min(-Vec3::dot(uv, n), 1.0f);
  Vec3 r_perp  = etai_over_etat * (uv + cos_theta * n);
  float disc   = 1.0f - r_perp.length_squared();
  if (disc < 0.0f) return Vec3(0, 0, 0);
  Vec3 r_par = -std::sqrt(disc) * n;
  return r_perp + r_par;
}

/**
 * @brief Stream output operator for debugging
 * @param out Output stream
 * @param v Vector to output
 * @return Reference to output stream
 */
std::ostream &operator<<(std::ostream &out, const Vec3 &v);
/// Type alias for 3D points
using Point3 = Vec3;

/// Type alias for RGB colors
using Color = Vec3;

}  // namespace kestrel
