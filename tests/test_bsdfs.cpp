#include "doctest.h"
#include "test_helpers.h"
#include "kestrel.h"

#include "bsdfs/lambertian.h"
#include "bsdfs/conductor.h"
#include "bsdfs/dielectric.h"
#include "bsdfs/microfacet.h"
#include "bsdfs/phong.h"
#include "bsdfs/blinn.h"
#include "bsdfs/blinn_microfacet.h"
#include "bsdfs/plastic.h"
#include "bsdfs/rough_dielectric.h"
#include "bsdfs/emissive.h"

using namespace kestrel;
using namespace kestrel_test;

namespace {

// A hit at the origin on a surface whose front face points +Y, hit by a ray
// coming down from above (front_face == true).
HitRecord make_hit() {
  HitRecord rec;
  rec.point = Point3(0, 0, 0);
  rec.t = 1.0f;
  rec.u = 0.5f;
  rec.v = 0.5f;
  rec.tangent = Vec3(1, 0, 0);
  Ray incoming(Point3(0, 1, 0), Vec3(0.2f, -1.0f, 0.1f).normalized());
  rec.set_face_normal(incoming, Vec3(0, 1, 0));
  return rec;
}

const Ray kIncoming(Point3(0, 1, 0), Vec3(0.2f, -1.0f, 0.1f).normalized());

// Invariants every BSDF's scatter() must respect when it does scatter.
void check_scatter_invariants(const Material &m) {
  PCG32 rng = make_rng();
  HitRecord rec = make_hit();
  for (int i = 0; i < 32; ++i) {
    Color atten(0, 0, 0);
    Ray scattered;
    float pdf_out = -1.0f;
    if (m.scatter(kIncoming, rec, atten, scattered, pdf_out, rng)) {
      CHECK(is_finite(atten));
      CHECK(atten.x >= 0.0f);
      CHECK(atten.y >= 0.0f);
      CHECK(atten.z >= 0.0f);
      CHECK(pdf_out >= 0.0f);
      CHECK(is_finite(scattered.direction));
      // A scatter must produce a non-degenerate direction.
      CHECK(scattered.direction.length() > 0.0f);
      if (m.is_specular()) {
        // Delta lobes signal themselves with pdf_out == 0 (no NEE/MIS).
        CHECK(pdf_out == doctest::Approx(0.0f));
      }
    }
  }
}

// ── Unbiasedness invariants ───────────────────────────────────────────────
//
// A Monte-Carlo BSDF estimator is only unbiased if its three methods agree:
//
//   1. Sampling consistency: when scatter() samples a direction wo with solid-
//      angle pdf p, the returned attenuation (the throughput weight) MUST equal
//      f(wi,wo)*|cos θo| / p, i.e. eval(wi,wo,n)*cos / pdf(wi,wo,n). If scatter()
//      and eval()/pdf() disagree, BSDF sampling and next-event estimation
//      integrate different functions and the MIS combination is biased — the
//      classic cause of a surface rendering too bright or too dark.
//
//   2. Energy conservation (white furnace): the directional-hemispherical
//      reflectance ρ(wi) = ∫ f(wi,wo) cos θo dωo must be ≤ 1 — a passive surface
//      cannot reflect more energy than it receives. The BSDF-sampling estimator
//      of ρ is exactly the average of the scatter() attenuation, so a mean
//      attenuation > 1 means the BSDF manufactures energy.

// Mean per-channel attenuation over many scatter() samples == MC estimate of the
// directional-hemispherical reflectance ρ(wi). Delta lobes still contribute
// their (1,1,1)-style weights, so this also bounds specular energy.
Color mean_attenuation(const Material &m, int n_samples = 40000) {
  PCG32 rng = make_rng();
  HitRecord rec = make_hit();
  Color sum(0, 0, 0);
  for (int i = 0; i < n_samples; ++i) {
    Color atten(0, 0, 0);
    Ray scattered;
    float pdf_out = -1.0f;
    if (m.scatter(kIncoming, rec, atten, scattered, pdf_out, rng))
      sum = sum + atten;
  }
  return sum / static_cast<float>(n_samples);
}

// Invariant 1: attenuation == eval*cos/pdf for every non-delta sample.
void check_sampling_consistency(const Material &m) {
  PCG32 rng = make_rng();
  HitRecord rec = make_hit();
  Vec3 n  = rec.normal;
  Vec3 wi = (kIncoming.direction * -1.0f).normalized();
  int checked = 0;
  for (int i = 0; i < 4000; ++i) {
    Color atten(0, 0, 0);
    Ray scattered;
    float pdf_out = -1.0f;
    if (!m.scatter(kIncoming, rec, atten, scattered, pdf_out, rng)) continue;
    if (pdf_out <= 1e-5f) continue;                 // delta lobe — invariant N/A
    Vec3 wo = scattered.direction.normalized();
    float cos = Vec3::dot(wo, n);
    if (cos <= 1e-4f) continue;                     // below the surface
    float p = m.pdf(wi, wo, n);
    if (p <= 1e-5f) continue;
    Color expect = m.eval(wi, wo, n) * (cos / p);
    // Combined absolute+relative tolerance (Release builds use -ffast-math).
    auto close = [](float a, float b) {
      return std::abs(a - b) <= 1e-3f + 5e-3f * std::max(std::abs(a), std::abs(b));
    };
    INFO("sample ", i, " pdf=", p, " cos=", cos);
    CHECK(close(atten.x, expect.x));
    CHECK(close(atten.y, expect.y));
    CHECK(close(atten.z, expect.z));
    ++checked;
  }
  CHECK(checked > 0);  // the BSDF must actually produce testable samples
}

}  // namespace

TEST_CASE("Lambertian: diffuse, non-specular, energy-conserving") {
  Lambertian m(Color(0.6f, 0.6f, 0.6f));
  CHECK(m.is_specular() == false);
  check_scatter_invariants(m);

  // get_color() folds in 1/pi, so albedo/pi <= albedo.
  Color c = m.get_color();
  CHECK(c.x == doctest::Approx(0.6f / 3.14159265f).epsilon(1e-3f));

  // Cosine-weighted pdf is non-negative and zero in the lower hemisphere.
  Vec3 n(0, 1, 0);
  CHECK(m.pdf(Vec3(0, 1, 0), Vec3(0, 1, 0), n) >= 0.0f);
  CHECK(m.pdf(Vec3(0, 1, 0), Vec3(0, -1, 0), n) == doctest::Approx(0.0f));
}

TEST_CASE("Conductor: perfect mirror reflection") {
  Conductor m(Color(0.9f, 0.8f, 0.7f));
  CHECK(m.is_specular() == true);
  check_scatter_invariants(m);

  // A ray going straight down reflects straight up off a +Y surface.
  HitRecord rec = make_hit();
  Ray down(Point3(0, 1, 0), Vec3(0, -1, 0));
  rec.set_face_normal(down, Vec3(0, 1, 0));
  PCG32 rng = make_rng();
  Color atten;
  Ray scattered;
  float pdf_out = -1.0f;
  REQUIRE(m.scatter(down, rec, atten, scattered, pdf_out, rng));
  check_vec_approx(scattered.direction.normalized(), Vec3(0, 1, 0), 1e-3f);
  check_vec_approx(atten, Color(0.9f, 0.8f, 0.7f));
  CHECK(pdf_out == doctest::Approx(0.0f));
}

TEST_CASE("Dielectric: specular, white attenuation") {
  Dielectric m(1.5f);
  CHECK(m.is_specular() == true);
  check_scatter_invariants(m);
  check_vec_approx(m.get_color(), Color(1, 1, 1));
}

TEST_CASE("Microfacet (rough conductor): specular flag, valid scatter") {
  Microfacet m(Color(1, 1, 1), 0.3f);
  CHECK(m.is_specular() == true);
  check_scatter_invariants(m);
}

TEST_CASE("RoughDielectric: specular flag, valid scatter") {
  RoughDielectric m(Color(1, 1, 1), 0.2f, 1.5f);
  CHECK(m.is_specular() == true);
  check_scatter_invariants(m);
}

TEST_CASE("Phong: glossy, non-negative eval and pdf") {
  Phong m(Color(0.5f, 0.5f, 0.5f), 32.0f);
  CHECK(m.is_specular() == false);
  check_scatter_invariants(m);

  Vec3 n(0, 1, 0);
  Vec3 wi(0, 1, 0), wo = Vec3(0.3f, 0.9f, 0.0f).normalized();
  Color f = m.eval(wi, wo, n);
  CHECK(is_finite(f));
  CHECK(f.x >= 0.0f);
  CHECK(m.pdf(wi, wo, n) >= 0.0f);
}

TEST_CASE("Blinn: glossy, non-negative eval and pdf") {
  Blinn m(Color(0.5f, 0.5f, 0.5f), 32.0f);
  CHECK(m.is_specular() == false);
  check_scatter_invariants(m);

  Vec3 n(0, 1, 0);
  Vec3 wi(0, 1, 0), wo = Vec3(0.3f, 0.9f, 0.0f).normalized();
  CHECK(m.eval(wi, wo, n).x >= 0.0f);
  CHECK(m.pdf(wi, wo, n) >= 0.0f);
}

TEST_CASE("BlinnMicrofacet: glossy, non-negative eval and pdf") {
  BlinnMicrofacet m(Color(0.5f, 0.5f, 0.5f), 32.0f);
  check_scatter_invariants(m);

  Vec3 n(0, 1, 0);
  Vec3 wi(0, 1, 0), wo = Vec3(0.3f, 0.9f, 0.0f).normalized();
  CHECK(m.eval(wi, wo, n).x >= 0.0f);
  CHECK(m.pdf(wi, wo, n) >= 0.0f);
}

TEST_CASE("Plastic: coated diffuse, valid scatter and eval") {
  Plastic m(Color(0.7f, 0.2f, 0.2f), 1.5f);
  check_scatter_invariants(m);

  Vec3 n(0, 1, 0);
  Vec3 wi(0, 1, 0), wo = Vec3(0.3f, 0.9f, 0.0f).normalized();
  CHECK(m.eval(wi, wo, n).x >= 0.0f);
  CHECK(m.pdf(wi, wo, n) >= 0.0f);
}

TEST_CASE("Emissive: emits radiance and terminates the path") {
  Emissive m(Color(3, 4, 5));
  check_vec_approx(m.emitted(), Color(3, 4, 5));
  // Non-scattering: scatter() returns false and signals a delta (pdf 0).
  HitRecord rec = make_hit();
  PCG32 rng = make_rng();
  Color atten;
  Ray scattered;
  float pdf_out = -1.0f;
  CHECK(m.scatter(kIncoming, rec, atten, scattered, pdf_out, rng) == false);
  CHECK(pdf_out == doctest::Approx(0.0f));
  // An emitter contributes no reflected color.
  check_vec_approx(m.get_color(), Color(0, 0, 0));
}

// ── Sampling consistency: attenuation == eval*cos/pdf (non-specular BSDFs) ──
// These BSDFs participate in NEE/MIS, so scatter() and eval()/pdf() must agree
// or the renderer is biased.
TEST_CASE("BSDF sampling consistency: scatter weight == eval*cos/pdf") {
  SUBCASE("Lambertian")      { Lambertian m(Color(0.6f, 0.5f, 0.4f));        check_sampling_consistency(m); }
  SUBCASE("Phong")           { Phong m(Color(0.5f, 0.4f, 0.3f), 32.0f);      check_sampling_consistency(m); }
  SUBCASE("Blinn")           { Blinn m(Color(0.5f, 0.4f, 0.3f), 32.0f);      check_sampling_consistency(m); }
  SUBCASE("BlinnMicrofacet") { BlinnMicrofacet m(Color(0.5f, 0.4f, 0.3f), 32.0f); check_sampling_consistency(m); }
  SUBCASE("Plastic")         { Plastic m(Color(0.6f, 0.5f, 0.4f), 1.5f);     check_sampling_consistency(m); }
  SUBCASE("Plastic high IOR"){ Plastic m(Color(0.24f, 0.77f, 0.36f), 1.9f);  check_sampling_consistency(m); }
}

// ── White furnace: a passive BSDF cannot reflect more energy than it receives.
// Mean scatter() attenuation == MC estimate of directional reflectance ρ(wi);
// it must be ≤ 1 in every channel (small tolerance for Monte-Carlo variance).
TEST_CASE("BSDF energy conservation: white reflectance does not gain energy") {
  auto rho_le_one = [](const Material &m) {
    Color rho = mean_attenuation(m);
    INFO("rho = (", rho.x, ", ", rho.y, ", ", rho.z, ")");
    CHECK(is_finite(rho));
    CHECK(rho.x <= 1.0f + 2e-2f);
    CHECK(rho.y <= 1.0f + 2e-2f);
    CHECK(rho.z <= 1.0f + 2e-2f);
  };
  SUBCASE("Lambertian")      { Lambertian m(Color(1, 1, 1));               rho_le_one(m); }
  SUBCASE("Phong")           { Phong m(Color(1, 1, 1), 32.0f);            rho_le_one(m); }
  SUBCASE("Blinn")           { Blinn m(Color(1, 1, 1), 32.0f);            rho_le_one(m); }
  SUBCASE("BlinnMicrofacet") { BlinnMicrofacet m(Color(1, 1, 1), 32.0f);  rho_le_one(m); }
  SUBCASE("Plastic 1.5")     { Plastic m(Color(1, 1, 1), 1.5f);           rho_le_one(m); }
  SUBCASE("Plastic 1.9")     { Plastic m(Color(1, 1, 1), 1.9f);           rho_le_one(m); }
  SUBCASE("Conductor")       { Conductor m(Color(1, 1, 1));               rho_le_one(m); }
  SUBCASE("Dielectric")      { Dielectric m(1.5f);                        rho_le_one(m); }
  SUBCASE("Microfacet")      { Microfacet m(Color(1, 1, 1), 0.3f);        rho_le_one(m); }
  SUBCASE("RoughDielectric") { RoughDielectric m(Color(1, 1, 1), 0.2f, 1.5f); rho_le_one(m); }
}
