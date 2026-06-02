/**
 * @file blinn_microfacet.h
 * @brief Blinn-Phong microfacet BRDF (F·D·G / 4 n·ωi n·ωo)
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
 * @class BlinnMicrofacet
 * @brief Cook-Torrance microfacet BRDF with a Blinn-Phong NDF.
 *
 * f = F·D·G / (4·(n·ωi)·(n·ωo)) with Blinn NDF D = (α+2)/(2π)·(n·h)^α, a
 * Beckmann-mapped Smith masking term G (the matching shadowing for the Blinn
 * NDF), and coloured Schlick Fresnel F (Ks acts as F0). The half-vector is
 * importance-sampled from the NDF.
 */
namespace kestrel {

class BlinnMicrofacet : public Material {
public:
  explicit BlinnMicrofacet(Color ks, float exponent)
      : texture(std::make_shared<ConstantTexture>(ks)),
        exponent(exponent) {}
  explicit BlinnMicrofacet(std::shared_ptr<const Texture> tex, float exponent)
      : texture(std::move(tex)), exponent(exponent) {}

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return texture->value(0, 0); }

  Color eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
             float u = 0.f, float v = 0.f) const override;

  float pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const override;

private:
  std::shared_ptr<const Texture> texture; ///< specular reflectance Ks (also Fresnel F0)
  float exponent;                         ///< Blinn exponent α

  // Smith G1 masking (Beckmann rational approximation, Walter et al. 2007),
  // with the Beckmann roughness derived from the Blinn exponent.
  float smith_g1(float cos_v) const;
};

}  // namespace kestrel
