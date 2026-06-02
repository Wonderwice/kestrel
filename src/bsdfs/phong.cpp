#include "phong.h"
#include "onb.h"
#include "constants.h"
#include "pcg32.h"

// Mirror reflection of the view direction wi about the normal n.
namespace kestrel {

static Vec3 reflect_about(const Vec3 &wi, const Vec3 &n) {
  return (2.0f * Vec3::dot(n, wi) * n - wi).normalized();
}

bool Phong::scatter(const Ray &incoming, const HitRecord &rec,
                    Color &attenuation, Ray &scattered,
                    float &pdf_out, PCG32 &rng) const {
  Vec3 wi = (incoming.direction * -1.0f).normalized();
  Vec3 r  = reflect_about(wi, rec.normal);

  // Sample a cosine-power lobe about the reflection direction r.
  float u1 = rng.next_float();
  float u2 = rng.next_float();
  float cos_a = std::pow(u1, 1.0f / (exponent + 1.0f));
  float sin_a = std::sqrt(std::max(0.0f, 1.0f - cos_a * cos_a));
  float phi   = TWO_PI * u2;

  Vec3 t, b;
  build_onb(r, t, b);
  Vec3 wo = (sin_a * std::cos(phi) * t + cos_a * r + sin_a * std::sin(phi) * b).normalized();

  float n_dot_wo = Vec3::dot(rec.normal, wo);
  if (n_dot_wo <= 0.0f) return false; // sampled below the surface

  // pdf of the lobe (r·wo == cos_a by construction).
  pdf_out = (exponent + 1.0f) * INV_PI * 0.5f * std::pow(cos_a, exponent);

  // f * cos / pdf reduces to Ks * cos; use the per-hit texture value here.
  attenuation = texture->value(rec.u, rec.v) * n_dot_wo;
  scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
  return true;
}

Color Phong::eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
                  float u, float v) const {
  Vec3 r = reflect_about(wi, n);
  float cos_lobe = std::max(0.0f, Vec3::dot(r, wo));
  float factor = (exponent + 1.0f) * INV_PI * 0.5f * std::pow(cos_lobe, exponent);
  return texture->value(u, v) * factor;
}

float Phong::pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const {
  Vec3 r = reflect_about(wi, n);
  float cos_lobe = std::max(0.0f, Vec3::dot(r, wo));
  return (exponent + 1.0f) * INV_PI * 0.5f * std::pow(cos_lobe, exponent);
}

}  // namespace kestrel
