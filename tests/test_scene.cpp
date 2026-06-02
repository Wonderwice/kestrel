#include "doctest.h"
#include "test_helpers.h"
#include "scene.h"
#include "camera.h"
#include "light.h"

#include "shapes/sphere.h"
#include "bsdfs/lambertian.h"
#include "bsdfs/emissive.h"

#include <memory>

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("Scene: builder accessors round-trip") {
  Scene scene;
  scene.set_background_color(Color(0.1f, 0.2f, 0.3f));
  scene.set_sample_count(16);
  scene.add_light(Light(Point3(1, 2, 3), Color(1, 1, 1)));
  scene.set_camera(std::make_unique<Camera>(
      Point3(0, 0, 0), Point3(0, 0, -1), Vec3(0, 1, 0), 90.0f, 64, 1.0f));

  check_vec_approx(scene.background_color(), Color(0.1f, 0.2f, 0.3f));
  CHECK(scene.sample_count() == 16);
  REQUIRE(scene.lights().size() == 1);
  check_vec_approx(scene.lights()[0].position, Point3(1, 2, 3));
  CHECK(scene.camera().width() == 64);
}

TEST_CASE("Scene: closest-hit and occlusion against two spheres") {
  Scene scene;
  auto *mat = new Lambertian(Color(0.8f, 0.8f, 0.8f));
  scene.add_bsdf(mat);
  scene.add_object(new Sphere(Point3(0, 0, -3), 0.5f, mat));
  scene.add_object(new Sphere(Point3(0, 0, -7), 0.5f, mat));
  scene.build();

  Ray ray(Point3(0, 0, 0), Vec3(0, 0, -1));
  HitRecord rec;
  REQUIRE(scene.hit(ray, 0.001f, 1e30f, rec));
  CHECK(rec.t == doctest::Approx(2.5f));            // nearest sphere
  CHECK(rec.material == mat);

  CHECK(scene.occluded(ray, 0.001f, 1e30f) == true);
  Ray up(Point3(0, 0, 0), Vec3(0, 1, 0));
  CHECK(scene.occluded(up, 0.001f, 1e30f) == false);
}

TEST_CASE("Scene: emissives() collects only emissive shapes") {
  Scene scene;
  auto *diffuse = new Lambertian(Color(0.8f, 0.8f, 0.8f));
  auto *light = new Emissive(Color(5, 5, 5));
  scene.add_bsdf(diffuse);
  scene.add_bsdf(light);
  scene.add_object(new Sphere(Point3(0, 0, -3), 0.5f, diffuse));
  scene.add_object(new Sphere(Point3(2, 0, -3), 0.5f, light));
  scene.build();

  REQUIRE(scene.emissives().size() == 1);
  CHECK(scene.emissives()[0]->material == light);
}

TEST_CASE("Scene: emissive_pdf is zero with no emitters") {
  Scene scene;
  auto *diffuse = new Lambertian(Color(0.8f, 0.8f, 0.8f));
  scene.add_bsdf(diffuse);
  scene.add_object(new Sphere(Point3(0, 0, -3), 0.5f, diffuse));
  scene.build();
  CHECK(scene.emissive_pdf(Point3(0, 0, 0), Vec3(0, 0, -1)) ==
        doctest::Approx(0.0f));
}
