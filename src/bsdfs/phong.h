/**
 * @file phong.h
 * @brief Normalized Phong glossy BRDF
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
 * @class Phong
 * @brief Normalized Phong reflection lobe about the mirror direction.
 *
 * BRDF: f = Ks · (α+1)/(2π) · max(r·ωo, 0)^α, where r is the mirror reflection
 * of the view direction about the shading normal. Importance-sampled by drawing
 * directions in a cosine-power lobe about r; the matching pdf lets the path
 * tracer MIS-weight it against light sampling.
 */
namespace kestrel {

class Phong : public Material {
public:
  explicit Phong(Color ks, float exponent)
      : texture(std::make_shared<ConstantTexture>(ks)),
        exponent(exponent) {}
  explicit Phong(std::shared_ptr<const Texture> tex, float exponent)
      : texture(std::move(tex)), exponent(exponent) {}

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return texture->value(0, 0); }

  Color eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
             float u = 0.f, float v = 0.f) const override;

  float pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const override;

private:
  std::shared_ptr<const Texture> texture; ///< specular reflectance Ks
  float exponent;                         ///< Phong exponent α
};

}  // namespace kestrel
