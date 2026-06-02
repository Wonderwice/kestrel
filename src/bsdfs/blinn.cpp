#include "blinn.h"
#include "onb.h"
#include "constants.h"
#include "pcg32.h"

// Schlick Fresnel with a coloured F0 (Ks).
namespace kestrel {

static Color fresnel_schlick(const Color &F0, float cos_theta) {
  float m = std::pow(std::max(0.0f, 1.0f - cos_theta), 5.0f);
  return F0 + (Color(1, 1, 1) - F0) * m;
}

Color Blinn::eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
                  float u, float v) const {
  Vec3 h = (wi + wo).normalized();
  float n_dot_h = std::max(0.0f, Vec3::dot(n, h));
  float o_dot_h = std::max(0.0f, Vec3::dot(wo, h));
  Color F = fresnel_schlick(texture->value(u, v), o_dot_h);
  float lobe = (exponent + 2.0f) / (8.0f * PI) * std::pow(n_dot_h, exponent);
  return F * lobe;
}

float Blinn::pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const {
  Vec3 h = (wi + wo).normalized();
  float n_dot_h = std::max(0.0f, Vec3::dot(n, h));
  float o_dot_h = Vec3::dot(wo, h);
  if (o_dot_h <= 0.0f) return 0.0f;
  float pdf_h = (exponent + 1.0f) * INV_PI * 0.5f * std::pow(n_dot_h, exponent);
  return pdf_h / (4.0f * o_dot_h); // half-vector -> outgoing Jacobian
}

bool Blinn::scatter(const Ray &incoming, const HitRecord &rec,
                    Color &attenuation, Ray &scattered,
                    float &pdf_out, PCG32 &rng) const {
  Vec3 wi = (incoming.direction * -1.0f).normalized();

  // Sample a half-vector in a cosine-power lobe about the normal.
  float u1 = rng.next_float();
  float u2 = rng.next_float();
  float cos_h = std::pow(u1, 1.0f / (exponent + 1.0f));
  float sin_h = std::sqrt(std::max(0.0f, 1.0f - cos_h * cos_h));
  float phi   = TWO_PI * u2;

  Vec3 t, b;
  build_onb(rec.normal, t, b);
  Vec3 h = (sin_h * std::cos(phi) * t + cos_h * rec.normal + sin_h * std::sin(phi) * b).normalized();

  // Reflect the view direction about the sampled half-vector.
  Vec3 wo = (2.0f * Vec3::dot(wi, h) * h - wi).normalized();
  float n_dot_wo = Vec3::dot(rec.normal, wo);
  if (n_dot_wo <= 0.0f) return false;

  pdf_out = pdf(wi, wo, rec.normal);
  if (pdf_out <= 0.0f) return false;

  Color f = eval(wi, wo, rec.normal, rec.u, rec.v);
  attenuation = f * (n_dot_wo / pdf_out);
  scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
  return true;
}

}  // namespace kestrel
