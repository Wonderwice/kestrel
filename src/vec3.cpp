#include "vec3.h"
#include "constants.h"

// NOTE: the hot Vec3 arithmetic (operators, dot, cross, length, normalized,
// reflect, refract, ...) now lives inline in vec3.h so it inlines into every
// caller. Only the non-hot helpers below remain out-of-line.

namespace kestrel {

Vec3 Vec3::random(float min, float max, PCG32 &rng) {
  return Vec3(rng.next_float() * (max - min) + min,
              rng.next_float() * (max - min) + min,
              rng.next_float() * (max - min) + min);
}

Vec3 Vec3::random_unit_vector(PCG32 &rng) {
  float a = rng.next_float() * (TWO_PI);
  float z = rng.next_float() * 2.0f - 1.0f;
  float r = std::sqrt(1.0f - z * z);
  return Vec3(r * std::cos(a), r * std::sin(a), z);
}

std::ostream &operator<<(std::ostream &out, const Vec3 &v) {
  return out << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

}  // namespace kestrel
