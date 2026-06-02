/**
 * @file plastic.h
 * @brief Coated diffuse (plastic): dielectric Fresnel coat over a Lambertian base
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "texture.h"
#include "vec3.h"
#include <memory>

/**
 * @class Plastic
 * @brief Specular dielectric coat over a diffuse base, blended by Fresnel.
 *
 * Models specular + (1−F)·diffuse with F the exact dielectric Fresnel reflectance. Each
 * scatter stochastically picks the specular coat (perfect mirror, a delta lobe)
 * with probability F, otherwise the cosine-weighted diffuse base. NEE/MIS see
 * only the diffuse lobe (the coat is a delta), weighted by (1−F).
 */
namespace kestrel {

class Plastic : public Material {
public:
  explicit Plastic(Color albedo, float eta = 1.5f, bool nonlinear = false)
      : texture(std::make_shared<ConstantTexture>(albedo)), nonlinear_(nonlinear) {
    set_eta(eta);
  }
  explicit Plastic(std::shared_ptr<const Texture> tex, float eta = 1.5f,
                   bool nonlinear = false)
      : texture(std::move(tex)), nonlinear_(nonlinear) {
    set_eta(eta);
  }

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return texture->value(0, 0); }

  Color eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
             float u = 0.f, float v = 0.f) const override;

  float pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const override;

private:
  std::shared_ptr<const Texture> texture; ///< diffuse base reflectance
  float eta;              ///< coat index of refraction (exterior = 1)
  float inv_eta2;         ///< 1/eta^2 radiance-scaling factor across the coat boundary
  float fdr_int;          ///< internal diffuse Fresnel reflectance F_dr = fresnelDiffuseReflectance(1/eta)
  bool  nonlinear_ = false; ///< Mitsuba's "nonlinear" flag: false (default) uses the
                            ///< scalar 1/(1-Fdr_int) series; true uses per-channel
                            ///< 1/(1-rho*Fdr_int) (colored internal scattering).

  void set_eta(float e) {
    eta = e;
    inv_eta2 = 1.0f / (e * e);
    // Egan-Hilgeman polynomial fit for the internal diffuse Fresnel reflectance
    // (== fresnelDiffuseReflectance(1/eta)), valid for eta >= 1. This is the
    // fraction of diffusely-scattered light reflected back at the interior
    // boundary -> it drives the internal multiple-scattering series
    // 1/(1 - rho*Fdr).
    fdr_int = -1.440f / (e * e) + 0.710f / e + 0.668f + 0.0636f * e;
  }

  // Exact unpolarized dielectric Fresnel reflectance at the air/coat boundary for a
  // ray arriving at angle cos_theta (>=0). eta = coat IOR (exterior = 1). Matches
  // Mitsuba's fresnel() rather than the Schlick approximation (notable for eta=1.9).
  float coat_fresnel(float cos_theta) const {
    cos_theta = std::min(std::max(cos_theta, 0.0f), 1.0f);
    float sin_t2 = (1.0f - cos_theta * cos_theta) * inv_eta2;  // (sin_i/eta)^2
    if (sin_t2 >= 1.0f) return 1.0f;                           // total internal reflection
    float cos_t = std::sqrt(1.0f - sin_t2);
    float rs = (cos_theta - eta * cos_t) / (cos_theta + eta * cos_t);
    float rp = (eta * cos_theta - cos_t) / (eta * cos_theta + cos_t);
    return 0.5f * (rs * rs + rp * rp);
  }
};

}  // namespace kestrel
