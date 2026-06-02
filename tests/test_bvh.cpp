#include "doctest.h"
#include "test_helpers.h"
#include "accel/bvh.h"
#include "shapes/sphere.h"

#include <algorithm>
#include <memory>
#include <vector>

using namespace kestrel;
using namespace kestrel_test;

namespace {

// Brute-force nearest hit over a primitive list, used as the oracle for the BVH.
bool brute_force_hit(const std::vector<const Shape *> &prims, const Ray &ray,
                     float t_min, float t_max, HitRecord &best) {
  bool hit_any = false;
  float closest = t_max;
  for (const Shape *p : prims) {
    HitRecord tmp;
    if (p->hit(ray, t_min, closest, tmp)) {
      hit_any = true;
      closest = tmp.t;
      best = tmp;
    }
  }
  return hit_any;
}

}  // namespace

TEST_CASE("BVH: default and empty-built hierarchies report empty") {
  BVH bvh;
  CHECK(bvh.empty());

  bvh.build({});
  CHECK(bvh.empty());
}

TEST_CASE("BVH: closest hit matches the brute-force oracle") {
  // Spheres staggered along -Z; owned here so they outlive the BVH.
  std::vector<std::unique_ptr<Sphere>> owned;
  owned.push_back(std::make_unique<Sphere>(Point3(0, 0, -3), 0.5f, nullptr));
  owned.push_back(std::make_unique<Sphere>(Point3(0, 0, -7), 0.5f, nullptr));
  owned.push_back(std::make_unique<Sphere>(Point3(0, 0, -5), 0.5f, nullptr));
  owned.push_back(std::make_unique<Sphere>(Point3(2, 0, -5), 0.5f, nullptr));

  std::vector<const Shape *> prims;
  for (auto &s : owned) prims.push_back(s.get());

  BVH bvh;
  bvh.build(prims);
  CHECK_FALSE(bvh.empty());

  Ray ray(Point3(0, 0, 0), Vec3(0, 0, -1));
  HitRecord bvh_rec, oracle_rec;
  bool bvh_hit = bvh.hit(ray, 0.001f, 1e30f, bvh_rec);
  bool oracle_hit = brute_force_hit(prims, ray, 0.001f, 1e30f, oracle_rec);

  REQUIRE(bvh_hit == oracle_hit);
  REQUIRE(bvh_hit);
  CHECK(bvh_rec.t == doctest::Approx(oracle_rec.t));   // nearest is the z=-3 sphere
  CHECK(bvh_rec.t == doctest::Approx(2.5f));
}

TEST_CASE("BVH: occluded agrees with any-hit") {
  std::vector<std::unique_ptr<Sphere>> owned;
  owned.push_back(std::make_unique<Sphere>(Point3(0, 0, -5), 1.0f, nullptr));
  std::vector<const Shape *> prims;
  for (auto &s : owned) prims.push_back(s.get());

  BVH bvh;
  bvh.build(prims);

  Ray blocked(Point3(0, 0, 0), Vec3(0, 0, -1));
  CHECK(bvh.occluded(blocked, 0.001f, 1e30f) == true);

  Ray clear(Point3(0, 0, 0), Vec3(0, 1, 0));
  CHECK(bvh.occluded(clear, 0.001f, 1e30f) == false);

  // A blocker beyond t_max does not occlude.
  CHECK(bvh.occluded(blocked, 0.001f, 1.0f) == false);
}

TEST_CASE("BVH: primitives() retains exactly the input set after reordering") {
  std::vector<std::unique_ptr<Sphere>> owned;
  for (int i = 0; i < 5; ++i)
    owned.push_back(std::make_unique<Sphere>(Point3(i * 2.0f, 0, -5), 0.5f, nullptr));

  std::vector<const Shape *> input;
  for (auto &s : owned) input.push_back(s.get());

  BVH bvh;
  bvh.build(input);

  const auto &after = bvh.primitives();
  CHECK(after.size() == input.size());

  // Same multiset of pointers (BVH may permute them, but loses/duplicates none).
  std::vector<const Shape *> a = input, b = after;
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());
  CHECK(a == b);
}
