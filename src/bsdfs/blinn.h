/**
 * @file blinn.h
 * @brief Normalized Blinn-Phong glossy BRDF (half-vector lobe with Fresnel)
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
 * @class Blinn
 * @brief Normalized Blinn-Phong reflection lobe about the half-vector.
 *
 * BRDF: f = F_h · (α+2)/(8π) · max(n·h, 0)^α, with Schlick Fresnel
 * F_h = Ks + (1−Ks)(1−(ωo·h))^5 (Ks acts as F0). Importance-sampled by drawing
 * a half-vector in a cosine-power lobe about the normal and reflecting the view
 * direction about it.
 */
namespace kestrel {

class Blinn : public Material {
public:
  explicit Blinn(Color ks, float exponent)
      : texture(std::make_shared<ConstantTexture>(ks)),
        exponent(exponent) {}
  explicit Blinn(std::shared_ptr<const Texture> tex, float exponent)
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
  float exponent;                         ///< Blinn-Phong exponent α
};

}  // namespace kestrel
