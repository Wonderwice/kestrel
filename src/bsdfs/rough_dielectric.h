/**
 * @file rough_dielectric.h
 * @brief Rough dielectric BSDF (GGX microfacet refraction, Walter et al. 2007)
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class RoughDielectric
 * @brief Microfacet glass: GGX-distributed normals with stochastic Fresnel
 *        reflection/refraction (Walter et al., "Microfacet Models for Refraction
 *        through Rough Surfaces").
 *
 * A micro-normal h is importance-sampled from the GGX NDF, the dielectric
 * Fresnel term at h decides reflection vs. refraction, and the surviving
 * throughput weight |wi·h|·G / (|wi·n|·|h·n|) is identical for both branches.
 * Treated as a delta-like lobe by the integrator (no NEE/MIS), matching the
 * smooth dielectric and conductor-microfacet materials.
 */
namespace kestrel {

class RoughDielectric : public Material {
public:
  RoughDielectric() : albedo(Color(1, 1, 1)),
                      alpha(0.1f), ior(1.5f) {}
  RoughDielectric(Color albedo, float roughness, float ior)
      : albedo(albedo),
        alpha(std::max(roughness, 1e-3f)), ior(ior) {}

  ~RoughDielectric() override {}

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return albedo; }

  bool is_specular() const override { return true; }

private:
  Color albedo; ///< specular reflectance/transmittance tint (white = clear glass)
  float alpha;  ///< GGX roughness
  float ior;    ///< interior index of refraction (exterior = 1)

  // Smith G1 masking (GGX height-correlated form).
  float smith_g1(float cos_v) const {
    float a2 = alpha * alpha;
    float v2 = cos_v * cos_v;
    return 2.0f * cos_v / (cos_v + std::sqrt(a2 + (1.0f - a2) * v2));
  }

  // Dielectric Fresnel via Schlick on the ior ratio.
  static float schlick(float cos_theta, float ratio) {
    float r0 = (1.0f - ratio) / (1.0f + ratio);
    r0 *= r0;
    return r0 + (1.0f - r0) * std::pow(std::max(0.0f, 1.0f - cos_theta), 5.0f);
  }
};

}  // namespace kestrel
