#include "doctest.h"
#include "test_helpers.h"
#include "light.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Light stores position and intensity") {
  Light l(Point3(1, 2, 3), Color(0.5f, 0.6f, 0.7f));
  check_vec_approx(l.position, Point3(1, 2, 3));
  check_vec_approx(l.get_intensity(), Color(0.5f, 0.6f, 0.7f));
}

TEST_CASE("Light::sample_direction returns a normalized vector toward the light") {
  Light l(Point3(0, 5, 0), Color(1, 1, 1));
  Vec3 dir = l.sample_direction(Point3(0, 0, 0));
  CHECK(is_unit_length(dir));
  // From the origin, the light is straight up.
  check_vec_approx(dir, Vec3(0, 1, 0));
}

TEST_CASE("Light::sample_direction points from the query point to the light") {
  Light l(Point3(4, 0, 0), Color(1, 1, 1));
  Vec3 dir = l.sample_direction(Point3(1, 0, 0));
  CHECK(is_unit_length(dir));
  check_vec_approx(dir, Vec3(1, 0, 0));
}
