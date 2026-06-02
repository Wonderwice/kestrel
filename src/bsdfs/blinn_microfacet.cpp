#include "blinn_microfacet.h"
#include "onb.h"
#include "constants.h"
#include "pcg32.h"

namespace kestrel {

static Color fresnel_schlick(const Color &F0, float cos_theta) {
  float m = std::pow(std::max(0.0f, 1.0f - cos_theta), 5.0f);
  return F0 + (Color(1, 1, 1) - F0) * m;
}

// Beckmann roughness m mapped from the Blinn exponent: α = 2/m² − 2.
float BlinnMicrofacet::smith_g1(float cos_v) const {
  cos_v = std::min(1.0f, std::max(1e-4f, cos_v));
  float m = std::sqrt(2.0f / (exponent + 2.0f));
  float sin_v = std::sqrt(std::max(0.0f, 1.0f - cos_v * cos_v));
  float tan_v = sin_v / cos_v;
  float a = 1.0f / (m * tan_v + 1e-6f);
  if (a >= 1.6f) return 1.0f;
  return (3.535f * a + 2.181f * a * a) / (1.0f + 2.276f * a + 2.577f * a * a);
}

Color BlinnMicrofacet::eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
                            float u, float v) const {
  float n_dot_wi = Vec3::dot(n, wi);
  float n_dot_wo = Vec3::dot(n, wo);
  if (n_dot_wi <= 0.0f || n_dot_wo <= 0.0f) return Color(0, 0, 0);

  Vec3 h = (wi + wo).normalized();
  float n_dot_h = std::max(0.0f, Vec3::dot(n, h));
  float o_dot_h = std::max(0.0f, Vec3::dot(wo, h));

  float D = (exponent + 2.0f) * INV_PI * 0.5f * std::pow(n_dot_h, exponent);
  float G = smith_g1(n_dot_wi) * smith_g1(n_dot_wo);
  Color F = fresnel_schlick(texture->value(u, v), o_dot_h);

  return F * (D * G / (4.0f * n_dot_wi * n_dot_wo));
}

float BlinnMicrofacet::pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const {
  Vec3 h = (wi + wo).normalized();
  float n_dot_h = std::max(0.0f, Vec3::dot(n, h));
  float o_dot_h = Vec3::dot(wo, h);
  if (o_dot_h <= 0.0f) return 0.0f;
  // Half-vector sampled proportional to (n·h)^α, converted to outgoing measure.
  float pdf_h = (exponent + 1.0f) * INV_PI * 0.5f * std::pow(n_dot_h, exponent);
  return pdf_h / (4.0f * o_dot_h);
}

bool BlinnMicrofacet::scatter(const Ray &incoming, const HitRecord &rec,
                              Color &attenuation, Ray &scattered,
                              float &pdf_out, PCG32 &rng) const {
  Vec3 wi = (incoming.direction * -1.0f).normalized();

  float u1 = rng.next_float();
  float u2 = rng.next_float();
  float cos_h = std::pow(u1, 1.0f / (exponent + 1.0f));
  float sin_h = std::sqrt(std::max(0.0f, 1.0f - cos_h * cos_h));
  float phi   = TWO_PI * u2;

  Vec3 t, b;
  build_onb(rec.normal, t, b);
  Vec3 h = (sin_h * std::cos(phi) * t + cos_h * rec.normal + sin_h * std::sin(phi) * b).normalized();

  Vec3 wo = (2.0f * Vec3::dot(wi, h) * h - wi).normalized();
  if (Vec3::dot(rec.normal, wo) <= 0.0f) return false;

  pdf_out = pdf(wi, wo, rec.normal);
  if (pdf_out <= 0.0f) return false;

  Color f = eval(wi, wo, rec.normal, rec.u, rec.v);
  float n_dot_wo = std::max(0.0f, Vec3::dot(rec.normal, wo));
  attenuation = f * (n_dot_wo / pdf_out);
  scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
  return true;
}

}  // namespace kestrel
