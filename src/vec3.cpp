#include "vec3.h"

Vec3::Vec3() : x(0), y(0), z(0) {}

Vec3::Vec3(float v) : x(v), y(v), z(v) {}

Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

Vec3 Vec3::operator+(const Vec3 &v) const {
  return Vec3(x + v.x, y + v.y, z + v.z);
}

Vec3 Vec3::operator-(const Vec3 &v) const {
  return Vec3(x - v.x, y - v.y, z - v.z);
}

/**
 * @brief Scalar multiplication
 * @param t Scalar value
 * @return This vector scaled by t
 */
Vec3 Vec3::operator*(float t) const { return Vec3(x * t, y * t, z * t); }

/**
 * @brief Component-wise multiplication
 * @param v Vector to multiply with
 * @return Component-wise product
 */
Vec3 Vec3::operator*(const Vec3 &v) const {
  return Vec3(x * v.x, y * v.y, z * v.z);
}

/**
 * @brief Scalar division
 * @param t Scalar divisor
 * @return This vector divided by t
 */
Vec3 Vec3::operator/(float t) const { return Vec3(x / t, y / t, z / t); }

/**
 * @brief In-place vector addition
 * @param v Vector to add
 * @return Reference to this vector after addition
 */
Vec3 &Vec3::operator+=(const Vec3 &v) {
  x += v.x;
  y += v.y;
  z += v.z;
  return *this;
}

/**
 * @brief In-place scalar multiplication
 * @param t Scalar multiplier
 * @return Reference to this vector after scaling
 */
Vec3 &Vec3::operator*=(float t) {
  x *= t;
  y *= t;
  z *= t;
  return *this;
}

/**
 * @brief Calculate the length (magnitude) of the vector
 * @return Euclidean length of the vector
 */
float Vec3::length() const { return std::sqrt(x * x + y * y + z * z); }

/**
 * @brief Calculate the squared length of the vector
 * @return Squared Euclidean length (avoids sqrt for performance)
 */
float Vec3::length_squared() const { return x * x + y * y + z * z; }

/**
 * @brief Get a unit-length version of this vector
 * @return Normalized vector with length 1
 */
Vec3 Vec3::normalized() const {
  float len = length();
  if (len == 0.0f) {
    // Avoid division by zero — return a safe default zero vector
    return Vec3(0.0f, 0.0f, 0.0f);
  }
  return Vec3(x / len, y / len, z / len);
}

/**
 * @brief Compute dot product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Scalar dot product a · b
 */
float Vec3::dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief Compute cross product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Vector perpendicular to both a and b (a × b)
 */
Vec3 Vec3::cross(const Vec3 &a, const Vec3 &b) {
  return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

/**
 * @brief Generate a random vector with each component in [min, max]
 * @param min Minimum component value
 * @param max Maximum component value
 * @return Random vector
 */
Vec3 Vec3::random(float min, float max) {
  PCG32 rng;
  return Vec3(rng.next_float() * (max - min) + min,
              rng.next_float() * (max - min) + min,
              rng.next_float() * (max - min) + min);
}

Vec3 Vec3::random_unit_vector() {
  PCG32 rng;
  float a = rng.next_float() * (2.0f * M_PI);
  float z = rng.next_float() * 2.0f - 1.0f;
  float r = std::sqrt(1.0f - z * z);
  return Vec3(r * std::cos(a), r * std::sin(a), z);
}

static Vec3 random_in_unit_sphere() {
  while (true) {
    Vec3 p = Vec3::random(-1.0f, 1.0f);
    if (p.length_squared() >= 1.0f)
      continue;
    return p;
  }
}

static Vec3 random_on_hemisphere(const Vec3 &normal) {
  Vec3 in_unit_sphere = random_in_unit_sphere();
  if (Vec3::dot(in_unit_sphere, normal) > 0.0f) {
    return in_unit_sphere;
  } else {
    return in_unit_sphere * -1.0f;
  }
}

Vec3 operator*(float t, const Vec3 &v) {
  return Vec3(t * v.x, t * v.y, t * v.z);
}

Vec3 reflect(const Vec3 &v, const Vec3 &n) {
  return v - 2.0f * Vec3::dot(v, n) * n;
}

std::ostream &operator<<(std::ostream &out, const Vec3 &v) {
  return out << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}
