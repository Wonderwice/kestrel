#include "doctest.h"
#include "test_helpers.h"
#include "camera.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Camera derives height from aspect ratio") {
  Camera cam(Point3(0, 0, 0), Point3(0, 0, -1), Vec3(0, 1, 0),
             90.0f, 800, 16.0f / 9.0f);
  CHECK(cam.width() == 800);
  CHECK(cam.height() == static_cast<int>(800 / (16.0f / 9.0f)));
}

TEST_CASE("Pinhole camera center ray points from look_from toward look_at") {
  Point3 look_from(0, 0, 0);
  Point3 look_at(0, 0, -1);
  Camera cam(look_from, look_at, Vec3(0, 1, 0), 90.0f, 100, 1.0f);

  PCG32 rng = make_rng();
  Ray r = cam.get_ray(0.5f, 0.5f, rng);  // center of the image

  // Zero aperture => the ray starts exactly at the camera position.
  check_vec_approx(r.origin, look_from);
  // Center ray looks straight down -Z.
  check_vec_approx(r.direction.normalized(), Vec3(0, 0, -1), 1e-3f);
}

TEST_CASE("Camera rays vary across the image plane") {
  Camera cam(Point3(0, 0, 0), Point3(0, 0, -1), Vec3(0, 1, 0),
             90.0f, 100, 1.0f);
  PCG32 rng = make_rng();

  Ray left = cam.get_ray(0.0f, 0.5f, rng);
  Ray right = cam.get_ray(1.0f, 0.5f, rng);
  // Left edge points toward -X relative to the right edge.
  CHECK(left.direction.normalized().x < right.direction.normalized().x);
}

TEST_CASE("Aperture jitters the ray origin within the lens radius") {
  float aperture = 2.0f;       // lens diameter
  float focal = 5.0f;
  Camera cam(Point3(0, 0, 0), Point3(0, 0, -1), Vec3(0, 1, 0),
             90.0f, 100, 1.0f, aperture, focal);
  PCG32 rng = make_rng();

  bool any_offset = false;
  for (int i = 0; i < 64; ++i) {
    Ray r = cam.get_ray(0.5f, 0.5f, rng);
    float lens_dist = r.origin.length();   // distance from camera center
    // Lens samples must lie within the aperture radius (aperture/2).
    CHECK(lens_dist <= aperture / 2.0f + 1e-3f);
    if (lens_dist > 1e-4f) any_offset = true;
  }
  CHECK(any_offset);  // DOF actually moves the origin off-center
}
