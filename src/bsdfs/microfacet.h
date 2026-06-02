/**
 * @file microfacet.h
 * @brief GGX (Trowbridge-Reitz) microfacet BSDF for rough conductors
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class Microfacet
 * @brief Rough conductor using GGX NDF with Smith masking-shadowing.
 *
 * Samples the GGX visible normal distribution and reflects the incoming
 * direction around the sampled half-vector. The returned attenuation already
 * folds in the BRDF/PDF ratio so the path tracer only needs to multiply
 * throughput by it.
 */
namespace kestrel {

class Microfacet : public Material {
public:
  Microfacet() : albedo(Color(1, 1, 1)), alpha(0.5f) {}
  Microfacet(Color albedo, float roughness)
      : albedo(albedo), alpha(std::max(roughness * roughness, 1e-4f)) {}

  ~Microfacet() override {}

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return albedo; }

  bool is_specular() const override { return true; }

private:
  Color albedo;
  float alpha; ///< GGX roughness parameter (alpha = roughness²)

  // Smith G1 term (GGX height-correlated form)
  static float smith_g1(float NdotV, float a) {
    float a2 = a * a;
    float v2 = NdotV * NdotV;
    return 2.0f * NdotV / (NdotV + std::sqrt(a2 + (1.0f - a2) * v2));
  }

  // Schlick Fresnel for a metallic F0 derived from albedo luminance
  static float schlick(float cos_theta, float F0) {
    return F0 + (1.0f - F0) * std::pow(1.0f - cos_theta, 5.0f);
  }
};

}  // namespace kestrel
