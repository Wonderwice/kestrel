#include "doctest.h"
#include "test_helpers.h"
#include "camera.h"
#include "integrator.h"
#include "direct_integrator.h"
#include "path_integrator.h"
#include "scene.h"

#include "shapes/sphere.h"
#include "bsdfs/lambertian.h"
#include "bsdfs/emissive.h"

#include <memory>
#include <utility>
#include <vector>

using namespace kestrel;
using namespace kestrel_test;

namespace {

// Exposes Integrator's protected power heuristic for direct testing.
struct MisProbe : Integrator {
  Color integrate(const Ray &, const Scene &, PCG32 &) const override {
    return Color(0, 0, 0);
  }
  static float power(float a, float b) { return mis_power(a, b); }
};

// A tiny scene: one diffuse sphere lit by a point light, on a known background.
std::unique_ptr<Scene> make_scene() {
  auto scene = std::make_unique<Scene>();
  scene->set_background_color(Color(0.25f, 0.5f, 0.75f));
  auto *mat = new Lambertian(Color(0.8f, 0.8f, 0.8f));
  scene->add_bsdf(mat);
  scene->add_object(new Sphere(Point3(0, 0, -3), 0.5f, mat));
  scene->add_light(Light(Point3(0, 5, -3), Color(10, 10, 10)));
  scene->build();
  return scene;
}

}  // namespace

TEST_CASE("Integrator::mis_power matches the power heuristic (beta=2)") {
  CHECK(MisProbe::power(1.0f, 1.0f) == doctest::Approx(0.5f));
  CHECK(MisProbe::power(2.0f, 1.0f) == doctest::Approx(0.8f));     // 4/5
  CHECK(MisProbe::power(1.0f, 0.0f) == doctest::Approx(1.0f));
  CHECK(MisProbe::power(0.0f, 1.0f) == doctest::Approx(0.0f));
}

TEST_CASE("DirectIntegrator: escaping ray returns the background color") {
  auto scene = make_scene();
  DirectIntegrator integ;
  PCG32 rng = make_rng();

  Ray escape(Point3(0, 0, 0), Vec3(0, 1, 0));  // points away into empty space
  check_vec_approx(integ.integrate(escape, *scene, rng), Color(0.25f, 0.5f, 0.75f));
}

TEST_CASE("DirectIntegrator: ray onto the lit sphere is finite and non-negative") {
  auto scene = make_scene();
  DirectIntegrator integ;
  PCG32 rng = make_rng();

  Ray onto(Point3(0, 0, 0), Vec3(0, 0, -1));
  Color L = integ.integrate(onto, *scene, rng);
  CHECK(is_finite(L));
  CHECK(L.x >= 0.0f);
  CHECK(L.y >= 0.0f);
  CHECK(L.z >= 0.0f);
}

TEST_CASE("PathIntegrator: escaping ray returns the background color") {
  auto scene = make_scene();
  PathIntegrator integ;
  PCG32 rng = make_rng();

  Ray escape(Point3(0, 0, 0), Vec3(0, 1, 0));
  check_vec_approx(integ.integrate(escape, *scene, rng), Color(0.25f, 0.5f, 0.75f));
}

TEST_CASE("PathIntegrator: zero max depth contributes no radiance") {
  auto scene = make_scene();
  PathIntegrator integ(0);
  PCG32 rng = make_rng();

  Ray onto(Point3(0, 0, 0), Vec3(0, 0, -1));
  check_vec_approx(integ.integrate(onto, *scene, rng), Color(0, 0, 0));
}

TEST_CASE("PathIntegrator: radiance stays finite and non-negative on a lit hit") {
  auto scene = make_scene();
  PathIntegrator integ(8);
  PCG32 rng = make_rng();

  Ray onto(Point3(0, 0, 0), Vec3(0, 0, -1));
  Color L = integ.integrate(onto, *scene, rng);
  CHECK(is_finite(L));
  CHECK(L.x >= 0.0f);
  CHECK(L.y >= 0.0f);
  CHECK(L.z >= 0.0f);
}

TEST_CASE("Integrator::render fills the buffer and reports progress") {
  auto scene = make_scene();
  // Camera aimed straight up into empty space, so every ray escapes and each
  // pixel resolves to the known background color.
  const int w = 8;
  Camera cam(Point3(0, 0, 0), Point3(0, 1, 0), Vec3(0, 0, -1), 60.0f, w, 1.0f);
  const int h = cam.height();

  DirectIntegrator integ;
  std::vector<Color> pixels(static_cast<size_t>(w) * h);

  std::vector<std::pair<int, int>> progress;
  integ.render(*scene, cam, /*spp=*/1, pixels, /*num_threads=*/1,
               [&](int done, int total) { progress.emplace_back(done, total); });

  // Every pixel is finite, non-negative, and equals the background.
  for (const Color &c : pixels) {
    CHECK(is_finite(c));
    CHECK(c.x >= 0.0f);
    check_vec_approx(c, Color(0.25f, 0.5f, 0.75f));
  }

  // The progress callback fired and its final call reports completion.
  REQUIRE(!progress.empty());
  CHECK(progress.back().first == h);
  CHECK(progress.back().second == h);

  // A second single-thread render is bit-for-bit identical (deterministic).
  std::vector<Color> pixels2(static_cast<size_t>(w) * h);
  integ.render(*scene, cam, /*spp=*/1, pixels2, /*num_threads=*/1);
  for (size_t i = 0; i < pixels.size(); ++i) {
    CHECK(pixels[i].x == pixels2[i].x);
    CHECK(pixels[i].y == pixels2[i].y);
    CHECK(pixels[i].z == pixels2[i].z);
  }
}
