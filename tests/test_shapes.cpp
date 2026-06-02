#include "doctest.h"
#include "test_helpers.h"
#include "kestrel.h"

#include "shapes/sphere.h"
#include "shapes/triangle.h"
#include "shapes/rectangle.h"

using namespace kestrel;
using namespace kestrel_test;

// ---------------------------------------------------------------------------
// Sphere
// ---------------------------------------------------------------------------
TEST_CASE("Sphere: ray hit records t, point and outward normal") {
  Sphere s(Point3(0, 0, -5), 1.0f, nullptr);

  Ray ray(Point3(0, 0, 0), Vec3(0, 0, -1));
  HitRecord rec;
  REQUIRE(s.hit(ray, 0.001f, 1e30f, rec));
  CHECK(rec.t == doctest::Approx(4.0f));               // hits near face at z=-4
  check_vec_approx(rec.point, Point3(0, 0, -4));
  check_vec_approx(rec.normal, Vec3(0, 0, 1));         // points back at the ray
  CHECK(rec.front_face == true);
}

TEST_CASE("Sphere: misses and behind-origin rejection") {
  Sphere s(Point3(0, 0, -5), 1.0f, nullptr);
  HitRecord rec;

  Ray miss(Point3(0, 0, 0), Vec3(0, 1, 0));
  CHECK(s.hit(miss, 0.001f, 1e30f, rec) == false);

  // Sphere is behind a ray pointing the other way.
  Ray behind(Point3(0, 0, 0), Vec3(0, 0, 1));
  CHECK(s.hit(behind, 0.001f, 1e30f, rec) == false);
}

TEST_CASE("Sphere: bounds and centroid") {
  Sphere s(Point3(1, 2, 3), 2.0f, nullptr);
  check_vec_approx(s.bounds().min, Point3(-1, 0, 1));
  check_vec_approx(s.bounds().max, Point3(3, 4, 5));
  check_vec_approx(s.centroid(), Point3(1, 2, 3));
}

TEST_CASE("Sphere: translate and scale move/resize the bounds") {
  Sphere s(Point3(0, 0, 0), 1.0f, nullptr);
  s.translate(Vec3(5, 0, 0));
  check_vec_approx(s.centroid(), Point3(5, 0, 0));

  s.scale(Vec3(2, 2, 2));   // radius *= cbrt(8) == 2
  check_vec_approx(s.bounds().min, Point3(3, -2, -2));
  check_vec_approx(s.bounds().max, Point3(7, 2, 2));
}

TEST_CASE("Sphere: light sampling yields a positive solid-angle pdf") {
  Sphere s(Point3(0, 0, -5), 1.0f, nullptr);
  PCG32 rng = make_rng();
  Vec3 sample_point;
  float pdf_sa = -1.0f;
  REQUIRE(s.sample_light(Point3(0, 0, 0), rng, sample_point, pdf_sa));
  CHECK(pdf_sa > 0.0f);

  // light_pdf agrees: positive toward the sphere, zero pointing away.
  CHECK(s.light_pdf(Point3(0, 0, 0), Vec3(0, 0, -1)) > 0.0f);
  CHECK(s.light_pdf(Point3(0, 0, 0), Vec3(0, 0, 1)) == doctest::Approx(0.0f));
}

// ---------------------------------------------------------------------------
// Triangle
// ---------------------------------------------------------------------------
namespace {
Triangle make_xy_triangle() {
  return Triangle(Point3(0, 0, 0), Point3(1, 0, 0), Point3(0, 1, 0),
                  Vec3(0, 0, 1), Vec3(0, 0, 1), Vec3(0, 0, 1), nullptr);
}
}  // namespace

TEST_CASE("Triangle: Moller-Trumbore hit and miss") {
  Triangle tri = make_xy_triangle();
  HitRecord rec;

  Ray hit(Point3(0.25f, 0.25f, 1.0f), Vec3(0, 0, -1));
  REQUIRE(tri.hit(hit, 0.001f, 1e30f, rec));
  CHECK(rec.t == doctest::Approx(1.0f));
  check_vec_approx(rec.point, Point3(0.25f, 0.25f, 0.0f));
  check_vec_approx(rec.normal, Vec3(0, 0, 1));
  CHECK(rec.front_face == true);

  // Outside the triangle (beyond the hypotenuse) misses.
  Ray miss(Point3(0.9f, 0.9f, 1.0f), Vec3(0, 0, -1));
  CHECK(tri.hit(miss, 0.001f, 1e30f, rec) == false);
}

TEST_CASE("Triangle: occluded matches hit for an in-range blocker") {
  Triangle tri = make_xy_triangle();
  Ray ray(Point3(0.25f, 0.25f, 1.0f), Vec3(0, 0, -1));
  CHECK(tri.occluded(ray, 0.001f, 1e30f) == true);
  // Blocker beyond t_max is not occluding.
  CHECK(tri.occluded(ray, 0.001f, 0.5f) == false);
}

TEST_CASE("Triangle: bounds and centroid") {
  Triangle tri = make_xy_triangle();
  check_vec_approx(tri.bounds().min, Point3(0, 0, 0));
  check_vec_approx(tri.bounds().max, Point3(1, 1, 0));
  check_vec_approx(tri.centroid(), Point3(0.5f, 0.5f, 0.0f));
}

TEST_CASE("Triangle: one-sided light sampling") {
  Triangle tri = make_xy_triangle();
  PCG32 rng = make_rng();
  Vec3 sample_point;
  float pdf_sa = -1.0f;

  // From the +Z (front) side the emitter is visible.
  REQUIRE(tri.sample_light(Point3(0.25f, 0.25f, 2.0f), rng, sample_point, pdf_sa));
  CHECK(pdf_sa > 0.0f);

  Vec3 toward = (Point3(0.25f, 0.25f, 0.0f) - Point3(0.25f, 0.25f, 2.0f)).normalized();
  CHECK(tri.light_pdf(Point3(0.25f, 0.25f, 2.0f), toward) > 0.0f);
}

TEST_CASE("Triangle: translate shifts the bounds") {
  Triangle tri = make_xy_triangle();
  tri.translate(Vec3(0, 0, 3));
  check_vec_approx(tri.bounds().min, Point3(0, 0, 3));
  check_vec_approx(tri.bounds().max, Point3(1, 1, 3));
}

// ---------------------------------------------------------------------------
// Rectangle (default: unit square in the XY plane at z=0)
// ---------------------------------------------------------------------------
TEST_CASE("Rectangle: hit at center with correct UVs") {
  Rectangle rect(nullptr);
  HitRecord rec;

  Ray ray(Point3(0, 0, 1), Vec3(0, 0, -1));
  REQUIRE(rect.hit(ray, 0.001f, 1e30f, rec));
  CHECK(rec.t == doctest::Approx(1.0f));
  check_vec_approx(rec.point, Point3(0, 0, 0));
  CHECK(rec.u == doctest::Approx(0.5f));
  CHECK(rec.v == doctest::Approx(0.5f));
  CHECK(std::abs(rec.normal.z) == doctest::Approx(1.0f));  // normal along Z
}

TEST_CASE("Rectangle: misses outside the quad") {
  Rectangle rect(nullptr);
  HitRecord rec;
  Ray ray(Point3(2, 2, 1), Vec3(0, 0, -1));  // hits the plane but outside extent
  CHECK(rect.hit(ray, 0.001f, 1e30f, rec) == false);
}

TEST_CASE("Rectangle: bounds and translate") {
  // Mitsuba convention: the default square spans [-1, 1] x [-1, 1].
  Rectangle rect(nullptr);
  check_vec_approx(rect.bounds().min, Point3(-1.0f, -1.0f, 0.0f));
  check_vec_approx(rect.bounds().max, Point3(1.0f, 1.0f, 0.0f));

  rect.translate(Vec3(1, 0, 0));
  check_vec_approx(rect.bounds().min, Point3(0.0f, -1.0f, 0.0f));
  check_vec_approx(rect.bounds().max, Point3(2.0f, 1.0f, 0.0f));
}

TEST_CASE("Rectangle: two-sided area-light sampling") {
  Rectangle rect(nullptr);  // [-1,1] square in the XY plane at z=0
  PCG32 rng = make_rng();
  Vec3 sample_point;
  float pdf_sa = -1.0f;

  REQUIRE(rect.sample_light(Point3(0, 0, 2), rng, sample_point, pdf_sa));
  CHECK(pdf_sa > 0.0f);
  CHECK(std::abs(sample_point.x) <= 1.0f);   // sample lies on the quad
  CHECK(std::abs(sample_point.y) <= 1.0f);
  CHECK(sample_point.z == doctest::Approx(0.0f));

  // Two-sided: visible (pdf > 0) from both the +Z and -Z sides.
  CHECK(rect.light_pdf(Point3(0, 0, 2), Vec3(0, 0, -1)) > 0.0f);
  CHECK(rect.light_pdf(Point3(0, 0, -2), Vec3(0, 0, 1)) > 0.0f);
  // A direction parallel to the plane misses it.
  CHECK(rect.light_pdf(Point3(0, 0, 2), Vec3(0, 1, 0)) == doctest::Approx(0.0f));
}
