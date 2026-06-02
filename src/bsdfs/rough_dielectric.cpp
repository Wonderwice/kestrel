#include "rough_dielectric.h"
#include "onb.h"
#include "constants.h"
#include "pcg32.h"

namespace kestrel {

bool RoughDielectric::scatter(const Ray &incoming, const HitRecord &rec,
                              Color &attenuation, Ray &scattered,
                              float &pdf_out, PCG32 &rng) const {
  pdf_out = 0.0f; // rough specular lobe: no NEE/MIS

  // Sample a micro-normal h from the GGX NDF about the (ray-opposing) normal.
  float u1 = rng.next_float();
  float u2 = rng.next_float();
  float phi    = TWO_PI * u1;
  float denom  = 1.0f + (alpha * alpha - 1.0f) * u2;
  float cos_th = std::sqrt(std::max(0.0f, (1.0f - u2) / std::max(denom, 1e-8f)));
  cos_th       = std::min(1.0f, cos_th);
  float sin_th = std::sqrt(std::max(0.0f, 1.0f - cos_th * cos_th));
  Vec3 h_local(sin_th * std::cos(phi), cos_th, sin_th * std::sin(phi));

  Vec3 t, b;
  build_onb(rec.normal, t, b);
  Vec3 h = (h_local.x * t + h_local.y * rec.normal + h_local.z * b).normalized();

  Vec3 in_dir = incoming.direction.normalized();
  Vec3 wi     = in_dir * -1.0f; // toward the viewer
  float cos_i = Vec3::dot(wi, h);
  if (cos_i <= 0.0f) return false; // micro-normal faces away

  // rec.normal already opposes the ray; pick the ior ratio by surface side.
  float ratio = rec.front_face ? (1.0f / ior) : ior;
  float sin_i = std::sqrt(std::max(0.0f, 1.0f - cos_i * cos_i));
  bool  tir   = ratio * sin_i > 1.0f;
  float F     = schlick(cos_i, ratio);

  Vec3 wo;
  if (tir || rng.next_float() < F) {
    wo = reflect(in_dir, h);
    if (Vec3::dot(wo, rec.normal) <= 0.0f) return false; // reflected below surface
  } else {
    wo = refract(in_dir, h, ratio);
    if (wo.length_squared() < 1e-12f) {              // numerical TIR fallback
      wo = reflect(in_dir, h);
      if (Vec3::dot(wo, rec.normal) <= 0.0f) return false;
    }
  }
  if (wo.length_squared() < 1e-12f) return false;     // guard degenerate direction
  wo = wo.normalized();

  // Walter et al. throughput weight (identical for reflection and refraction).
  float NdotV = std::abs(Vec3::dot(wi, rec.normal));
  float NdotL = std::abs(Vec3::dot(wo, rec.normal));
  float NdotH = std::abs(Vec3::dot(h, rec.normal));
  if (NdotV < 1e-6f || NdotH < 1e-6f) return false;

  float G      = smith_g1(NdotV) * smith_g1(NdotL);
  float weight = std::abs(cos_i) * G / (NdotV * NdotH);

  attenuation = albedo * weight;

  // Offset along the outgoing hemisphere to avoid self-intersection.
  float side = (Vec3::dot(wo, rec.normal) > 0.0f) ? 1.0f : -1.0f;
  scattered  = Ray(rec.point + side * rec.normal * RAY_EPSILON, wo);
  return true;
}

}  // namespace kestrel
