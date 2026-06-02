// Shared helpers for the kestrel unit tests.
//
// The library is compiled with -ffast-math in Release, so computed floats must
// never be compared with ==. Everything here routes through doctest::Approx or
// explicit epsilons.
#pragma once

#include "doctest.h"
#include "vec3.h"

namespace kestrel_test {

using namespace kestrel;

// Component-wise approximate equality for Vec3 (and its Point3/Color aliases).
inline void check_vec_approx(const Vec3 &got, const Vec3 &want,
                             float eps = 1e-4f) {
  CHECK(got.x == doctest::Approx(want.x).epsilon(eps));
  CHECK(got.y == doctest::Approx(want.y).epsilon(eps));
  CHECK(got.z == doctest::Approx(want.z).epsilon(eps));
}

inline bool is_unit_length(const Vec3 &v, float eps = 1e-4f) {
  return std::abs(v.length() - 1.0f) < eps;
}

inline bool is_finite(const Vec3 &v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

// A fixed-seed RNG so RNG-dependent tests are deterministic, mirroring how the
// renderer seeds each thread.
inline PCG32 make_rng(uint64_t seed = 1234u, uint64_t stream = 1u) {
  return PCG32(seed, stream);
}

}  // namespace kestrel_test
