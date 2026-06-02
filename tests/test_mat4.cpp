#include "doctest.h"
#include "test_helpers.h"
#include "math/mat4.h"
#include "constants.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Mat4::identity leaves points and vectors unchanged") {
  Mat4 I = Mat4::identity();
  check_vec_approx(I.transform_point(Point3(3, -2, 7)), Point3(3, -2, 7));
  check_vec_approx(I.transform_vector(Vec3(3, -2, 7)), Vec3(3, -2, 7));
}

TEST_CASE("Mat4::translation moves points but not vectors") {
  Mat4 T = Mat4::translation(Vec3(1, 2, 3));
  check_vec_approx(T.transform_point(Point3(0, 0, 0)), Point3(1, 2, 3));
  check_vec_approx(T.transform_point(Point3(5, 5, 5)), Point3(6, 7, 8));
  // A direction ignores translation.
  check_vec_approx(T.transform_vector(Vec3(1, 0, 0)), Vec3(1, 0, 0));
}

TEST_CASE("Mat4::scaling scales each axis") {
  Mat4 S = Mat4::scaling(Vec3(2, 3, 4));
  check_vec_approx(S.transform_point(Point3(1, 1, 1)), Point3(2, 3, 4));
  check_vec_approx(S.transform_vector(Vec3(1, 1, 1)), Vec3(2, 3, 4));
}

TEST_CASE("Mat4::rotation rotates about an axis") {
  // 90 degrees about +Z sends +X to +Y.
  Mat4 R = Mat4::rotation(90.0f, Vec3(0, 0, 1));
  check_vec_approx(R.transform_vector(Vec3(1, 0, 0)), Vec3(0, 1, 0), 1e-3f);
  // Points on the rotation axis are fixed.
  check_vec_approx(R.transform_point(Point3(0, 0, 5)), Point3(0, 0, 5), 1e-3f);
}

TEST_CASE("Mat4 product composes transforms (translate then scale)") {
  Mat4 S = Mat4::scaling(Vec3(2, 2, 2));
  Mat4 T = Mat4::translation(Vec3(1, 0, 0));

  // (S * T) applies T first then S in row-major point convention.
  Mat4 ST = S * T;
  check_vec_approx(ST.transform_point(Point3(0, 0, 0)), Point3(2, 0, 0));

  // (T * S) applies S first then T.
  Mat4 TS = T * S;
  check_vec_approx(TS.transform_point(Point3(1, 0, 0)), Point3(3, 0, 0));
}

TEST_CASE("Mat4::transform_normal uses the inverse-transpose and renormalizes") {
  // Under a non-uniform scale, a normal must transform by the inverse-transpose,
  // not the matrix itself. Scaling x by 2 should *shrink* the x-component of a
  // surface normal relative to a naive transform, and the result is unit length.
  Mat4 S = Mat4::scaling(Vec3(2, 1, 1));

  // Normal of a plane tilted 45 degrees in the XY plane.
  Vec3 n = Vec3(1, 1, 0).normalized();
  Vec3 tn = S.transform_normal(n);
  CHECK(is_unit_length(tn));
  // Inverse-transpose of diag(2,1,1) is diag(0.5,1,1): x shrinks relative to y.
  CHECK(std::abs(tn.x) < std::abs(tn.y));
}
