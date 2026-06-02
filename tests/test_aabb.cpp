#include "doctest.h"
#include "test_helpers.h"
#include "accel/aabb.h"
#include "ray.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("AABB default-constructs empty (min > max)") {
  AABB box;
  CHECK(box.min.x > box.max.x);
  CHECK(box.min.y > box.max.y);
  CHECK(box.min.z > box.max.z);
}

TEST_CASE("AABB::expand grows to include points") {
  AABB box;
  box.expand(Point3(1, 2, 3));
  check_vec_approx(box.min, Point3(1, 2, 3));
  check_vec_approx(box.max, Point3(1, 2, 3));

  box.expand(Point3(-1, 5, 0));
  check_vec_approx(box.min, Point3(-1, 2, 0));
  check_vec_approx(box.max, Point3(1, 5, 3));
}

TEST_CASE("AABB::expand merges another box") {
  AABB a;
  a.expand(Point3(0, 0, 0));
  a.expand(Point3(1, 1, 1));

  AABB b;
  b.expand(Point3(2, -1, 0.5f));
  b.expand(Point3(3, 0, 4));

  a.expand(b);
  check_vec_approx(a.min, Point3(0, -1, 0));
  check_vec_approx(a.max, Point3(3, 1, 4));
}

TEST_CASE("AABB::longest_axis picks the largest extent") {
  AABB box;
  box.expand(Point3(0, 0, 0));
  box.expand(Point3(1, 5, 2));
  CHECK(box.longest_axis() == 1);  // y is longest

  AABB box2;
  box2.expand(Point3(0, 0, 0));
  box2.expand(Point3(10, 1, 2));
  CHECK(box2.longest_axis() == 0);  // x is longest

  AABB box3;
  box3.expand(Point3(0, 0, 0));
  box3.expand(Point3(1, 2, 9));
  CHECK(box3.longest_axis() == 2);  // z is longest
}

TEST_CASE("AABB::intersect slab test") {
  AABB box;
  box.expand(Point3(-1, -1, -1));
  box.expand(Point3(1, 1, 1));

  // Ray heading straight into the box from outside hits.
  Ray hit(Point3(-5, 0, 0), Vec3(1, 0, 0));
  CHECK(box.intersect(hit, 0.0f, 1e30f) == true);

  // Parallel ray that passes above the box misses.
  Ray miss(Point3(-5, 5, 0), Vec3(1, 0, 0));
  CHECK(box.intersect(miss, 0.0f, 1e30f) == false);

  // Box entirely behind the ray's [tMin,tMax] window is rejected.
  Ray behind(Point3(5, 0, 0), Vec3(1, 0, 0));
  CHECK(box.intersect(behind, 0.0f, 1e30f) == false);

  // Ray originating inside the box still reports an intersection.
  Ray inside(Point3(0, 0, 0), Vec3(1, 0, 0));
  CHECK(box.intersect(inside, 0.0f, 1e30f) == true);
}
