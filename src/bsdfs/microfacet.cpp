#include "microfacet.h"
#include "onb.h"
#include "constants.h"
#include "pcg32.h"

namespace kestrel {

bool Microfacet::scatter(const Ray &incoming, const HitRecord &rec,
                          Color &attenuation, Ray &scattered,
                          float &pdf_out, PCG32 &rng) const {
  // Treated as a specular/delta lobe by the integrator (no NEE/MIS), matching
  // the prior behaviour where pdf() was not overridden.
  pdf_out = 0.0f;
  float u1 = rng.next_float();
  float u2 = rng.next_float();

  // Sample the GGX NDF to get a micro-normal (half-vector) in local space.
  float phi      = TWO_PI * u1;
  float cos_th   = std::sqrt((1.0f - u2) / (1.0f + (alpha * alpha - 1.0f) * u2));
  float sin_th   = std::sqrt(std::max(0.0f, 1.0f - cos_th * cos_th));
  Vec3 h_local(sin_th * std::cos(phi), cos_th, sin_th * std::sin(phi));

  // Transform to world space using surface normal as the Y axis.
  Vec3 tangent, bitangent;
  build_onb(rec.normal, tangent, bitangent);
  Vec3 h = (h_local.x * tangent + h_local.y * rec.normal + h_local.z * bitangent).normalized();

  // Reflect the incoming direction around the half-vector.
  Vec3 wi = (incoming.direction * -1.0f).normalized();
  float VdotH = Vec3::dot(wi, h);
  Vec3 wo = (2.0f * VdotH * h - wi).normalized();

  // Below-surface reflection: discard.
  float NdotL = Vec3::dot(wo, rec.normal);
  if (NdotL <= 0.0f) return false;

  float NdotV = std::max(Vec3::dot(wi, rec.normal), 1e-6f);
  float NdotH = std::max(Vec3::dot(rec.normal, h), 1e-6f);
  VdotH       = std::max(VdotH, 1e-6f);

  // Luminance of albedo as the metallic F0.
  float lum = 0.2126f * albedo.x + 0.7152f * albedo.y + 0.0722f * albedo.z;
  float F   = schlick(VdotH, std::max(lum, 0.04f));

  // Smith masking-shadowing.
  float G = smith_g1(NdotV, alpha) * smith_g1(NdotL, alpha);

  // Weight = F * G * |wi·h| / (|wi·n| * |h·n|)
  // This is the cancellation of the GGX NDF from numerator and pdf.
  float weight = F * G * VdotH / (NdotV * NdotH);

  attenuation = albedo * weight;
  scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
  return true;
}

}  // namespace kestrel
