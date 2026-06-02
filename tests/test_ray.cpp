#include "doctest.h"
#include "test_helpers.h"
#include "kestrel.h"
#include "ray.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Ray stores origin/direction and precomputes inv_direction") {
  Ray r(Point3(1, 2, 3), Vec3(2, 4, 8));
  check_vec_approx(r.origin, Point3(1, 2, 3));
  check_vec_approx(r.direction, Vec3(2, 4, 8));
  check_vec_approx(r.inv_direction, Vec3(0.5f, 0.25f, 0.125f));
}

TEST_CASE("Ray::at evaluates the parametric equation") {
  Ray r(Point3(0, 0, 0), Vec3(1, 0, 0));
  check_vec_approx(r.at(0.0f), Point3(0, 0, 0));
  check_vec_approx(r.at(5.0f), Point3(5, 0, 0));

  Ray r2(Point3(1, 1, 1), Vec3(0, 2, 0));
  check_vec_approx(r2.at(3.0f), Point3(1, 7, 1));
}

TEST_CASE("HitRecord::set_face_normal orients the normal against the ray") {
  Vec3 outward(0, 1, 0);

  // Ray travelling downward hits the front face; normal stays outward.
  Ray down(Point3(0, 1, 0), Vec3(0, -1, 0));
  HitRecord front;
  front.set_face_normal(down, outward);
  CHECK(front.front_face == true);
  check_vec_approx(front.normal, Vec3(0, 1, 0));

  // Ray travelling upward hits the back face; normal is flipped to face it.
  Ray up(Point3(0, -1, 0), Vec3(0, 1, 0));
  HitRecord back;
  back.set_face_normal(up, outward);
  CHECK(back.front_face == false);
  check_vec_approx(back.normal, Vec3(0, -1, 0));
}

TEST_CASE("HitRecord defaults are zeroed") {
  HitRecord rec;
  CHECK(rec.t == 0.0f);
  CHECK(rec.u == 0.0f);
  CHECK(rec.v == 0.0f);
  CHECK(rec.front_face == false);
  CHECK(rec.material == nullptr);
}
