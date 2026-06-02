#include "doctest.h"
#include "test_helpers.h"
#include "vec3.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Vec3 construction") {
  CHECK(Vec3().x == 0.0f);
  check_vec_approx(Vec3(), Vec3(0, 0, 0));
  check_vec_approx(Vec3(2.0f), Vec3(2, 2, 2));   // broadcast constructor
  check_vec_approx(Vec3(1, 2, 3), Vec3(1, 2, 3));
}

TEST_CASE("Vec3 arithmetic operators") {
  Vec3 a(1, 2, 3), b(4, 5, 6);
  check_vec_approx(a + b, Vec3(5, 7, 9));
  check_vec_approx(b - a, Vec3(3, 3, 3));
  check_vec_approx(a * 2.0f, Vec3(2, 4, 6));
  check_vec_approx(2.0f * a, Vec3(2, 4, 6));     // scalar on the left
  check_vec_approx(a * b, Vec3(4, 10, 18));      // component-wise
  check_vec_approx(b / 2.0f, Vec3(2, 2.5f, 3));
  check_vec_approx(-a, Vec3(-1, -2, -3));
}

TEST_CASE("Vec3 compound assignment") {
  Vec3 v(1, 2, 3);
  v += Vec3(1, 1, 1);
  check_vec_approx(v, Vec3(2, 3, 4));
  v *= 2.0f;
  check_vec_approx(v, Vec3(4, 6, 8));
  v *= Vec3(0.5f, 0.5f, 0.5f);
  check_vec_approx(v, Vec3(2, 3, 4));
  v /= 2.0f;
  check_vec_approx(v, Vec3(1, 1.5f, 2));
}

TEST_CASE("Vec3 indexing reads and writes the right components") {
  Vec3 v(7, 8, 9);
  CHECK(v[0] == 7.0f);
  CHECK(v[1] == 8.0f);
  CHECK(v[2] == 9.0f);
  v[1] = 42.0f;
  CHECK(v.y == 42.0f);
}

TEST_CASE("Vec3 length and normalization") {
  Vec3 v(3, 4, 0);
  CHECK(v.length() == doctest::Approx(5.0f));
  CHECK(v.length_squared() == doctest::Approx(25.0f));

  Vec3 n = v.normalized();
  CHECK(is_unit_length(n));
  check_vec_approx(n, Vec3(0.6f, 0.8f, 0.0f));

  // Zero-vector guard: normalized() must not divide by zero (vec3.h).
  Vec3 z = Vec3(0, 0, 0).normalized();
  check_vec_approx(z, Vec3(0, 0, 0));
}

TEST_CASE("Vec3 dot and cross") {
  CHECK(Vec3::dot(Vec3(1, 2, 3), Vec3(4, 5, 6)) == doctest::Approx(32.0f));
  CHECK(Vec3::dot(Vec3(1, 0, 0), Vec3(0, 1, 0)) == doctest::Approx(0.0f));

  // Right-handed basis: x cross y = z.
  check_vec_approx(Vec3::cross(Vec3(1, 0, 0), Vec3(0, 1, 0)), Vec3(0, 0, 1));
  // Cross product is orthogonal to both inputs.
  Vec3 a(1, 2, 3), b(-2, 0, 5);
  Vec3 c = Vec3::cross(a, b);
  CHECK(Vec3::dot(c, a) == doctest::Approx(0.0f));
  CHECK(Vec3::dot(c, b) == doctest::Approx(0.0f));
}

TEST_CASE("Vec3 free functions: min, max, reflect") {
  check_vec_approx(min(Vec3(1, 5, 3), Vec3(4, 2, 6)), Vec3(1, 2, 3));
  check_vec_approx(max(Vec3(1, 5, 3), Vec3(4, 2, 6)), Vec3(4, 5, 6));

  // Reflecting a downward ray off a flat +Y surface flips only the Y term.
  check_vec_approx(reflect(Vec3(1, -1, 0), Vec3(0, 1, 0)), Vec3(1, 1, 0));
}

TEST_CASE("Vec3 refract") {
  // Straight-down ray through a flat interface stays straight when eta == 1.
  Vec3 d(0, -1, 0), n(0, 1, 0);
  check_vec_approx(refract(d, n, 1.0f), Vec3(0, -1, 0));

  // Total internal reflection returns the zero vector (disc < 0). A grazing ray
  // going into a much-less-dense medium triggers it.
  Vec3 grazing = Vec3(1, -0.01f, 0).normalized();
  Vec3 r = refract(grazing, n, 5.0f);
  check_vec_approx(r, Vec3(0, 0, 0));
}

TEST_CASE("Vec3::random stays within bounds and random_unit_vector is unit") {
  PCG32 rng = make_rng();
  for (int i = 0; i < 64; ++i) {
    Vec3 r = Vec3::random(-2.0f, 3.0f, rng);
    CHECK(r.x >= -2.0f);
    CHECK(r.x <= 3.0f);
    CHECK(r.y >= -2.0f);
    CHECK(r.y <= 3.0f);
    CHECK(r.z >= -2.0f);
    CHECK(r.z <= 3.0f);

    Vec3 u = Vec3::random_unit_vector(rng);
    CHECK(is_unit_length(u, 1e-3f));
  }
}
