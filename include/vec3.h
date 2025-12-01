/**
 * @file vec3.h
 * @brief 3D vector mathematics for CPU and GPU execution
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef VEC3_H
#define VEC3_H

#include "pcg32.h"
#include <cmath>
#include <iostream>

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

/**
 * @class Vec3
 * @brief A 3D vector class supporting both CPU and GPU execution
 *
 * This class represents a 3D vector with x, y, z components. All methods are
 * decorated with HOST_DEVICE to enable compilation for both CPU and CUDA GPU.
 * Used for positions, directions, colors, and normals throughout the renderer.
 */
class Vec3 {
public:
  float x, y, z; ///< Vector components

  /**
   * @brief Default constructor - initializes to zero vector
   */
  HOST_DEVICE Vec3();

  /**
   * @brief Construct vector with all components set to the same value
   * @param v Value for x, y, and z components
   */
  HOST_DEVICE Vec3(float v);

  /**
   * @brief Construct vector from three components
   * @param x X component
   * @param y Y component
   * @param z Z component
   */
  HOST_DEVICE Vec3(float x, float y, float z);
  /**
   * @brief Vector addition
   * @param v Vector to add
   * @return Sum of this vector and v
   */
  HOST_DEVICE Vec3 operator+(const Vec3 &v) const;

  /**
   * @brief Vector subtraction
   * @param v Vector to subtract
   * @return Difference of this vector and v
   */
  HOST_DEVICE Vec3 operator-(const Vec3 &v) const;

  /**
   * @brief Scalar multiplication
   * @param t Scalar value
   * @return This vector scaled by t
   */
  HOST_DEVICE Vec3 operator*(float t) const;

  /**
   * @brief Component-wise multiplication
   * @param v Vector to multiply with
   * @return Component-wise product
   */
  HOST_DEVICE Vec3 operator*(const Vec3 &v) const;

  /**
   * @brief Scalar division
   * @param t Scalar divisor
   * @return This vector divided by t
   */
  HOST_DEVICE Vec3 operator/(float t) const;

  /**
   * @brief In-place vector addition
   * @param v Vector to add
   * @return Reference to this vector after addition
   */
  HOST_DEVICE Vec3 &operator+=(const Vec3 &v);

  /**
   * @brief In-place scalar multiplication
   * @param t Scalar multiplier
   * @return Reference to this vector after scaling
   */
  HOST_DEVICE Vec3 &operator*=(float t);

  /**
   * @brief Calculate the length (magnitude) of the vector
   * @return Euclidean length of the vector
   */
  HOST_DEVICE float length() const;

  /**
   * @brief Calculate the squared length of the vector
   * @return Squared Euclidean length (avoids sqrt for performance)
   */
  HOST_DEVICE float length_squared() const;

  /**
   * @brief Get a unit-length version of this vector
   * @return Normalized vector with length 1
   */
  HOST_DEVICE Vec3 normalized() const;

  /**
   * @brief Compute dot product of two vectors
   * @param a First vector
   * @param b Second vector
   * @return Scalar dot product a · b
   */
  HOST_DEVICE static float dot(const Vec3 &a, const Vec3 &b);

  /**
   * @brief Compute cross product of two vectors
   * @param a First vector
   * @param b Second vector
   * @return Vector perpendicular to both a and b (a × b)
   */
  HOST_DEVICE static Vec3 cross(const Vec3 &a, const Vec3 &b);

  /**
   * @brief Generate a random vector with each component in [min, max]
   * @param min Minimum component value
   * @param max Maximum component value
   * @return Random vector
   */
  HOST_DEVICE static Vec3 random(float min, float max);

  HOST_DEVICE static Vec3 random_unit_vector();

  HOST_DEVICE static Vec3 random_in_unit_sphere();

  HOST_DEVICE static Vec3 random_on_hemisphere(const Vec3 &normal);
};

/**
 * @brief Scalar multiplication with scalar on left side
 * @param t Scalar multiplier
 * @param v Vector to multiply
 * @return Scaled vector
 */
HOST_DEVICE Vec3 operator*(float t, const Vec3 &v);

/**
 * @brief Reflect vector v around normal n
 * @param v Incoming vector
 * @param n Normal vector
 * @return Reflected vector
 */
HOST_DEVICE Vec3 reflect(const Vec3 &v, const Vec3 &n);

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

#endif