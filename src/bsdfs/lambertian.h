/**
 * @file lambertian.h
 * @brief Lambertian diffuse reflection model for ray tracing
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "ray.h"
#include "texture.h"
#include "vec3.h"
#include <memory>

/**
 * @class Lambertian
 * @brief Implements Lambertian diffuse reflection
 *
 * This class provides methods to compute diffuse reflection directions
 * based on the Lambertian reflection model. It generates random
 * directions over the hemisphere oriented around the surface normal.
 */
namespace kestrel {

class Lambertian : public Material {
public:
  Lambertian()
      : texture(std::make_shared<ConstantTexture>(Color(0.8f, 0.8f, 0.8f))) {}
  explicit Lambertian(Color c)
      : texture(std::make_shared<ConstantTexture>(c)) {}
  explicit Lambertian(std::shared_ptr<const Texture> tex)
      : texture(std::move(tex)) {}
  void set_normal_map(std::shared_ptr<const Texture> nm) { normal_map = std::move(nm); }

  /**
   * @brief Generate a diffuse scatter direction
   * @param incoming Incoming ray direction
   * @param rec Hit record with surface normal
   * @param attenuation Output color attenuation
   * @param scattered Output scattered ray
   * @return True if scattering occurred
   */
  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  /**
   * @brief Get the color of the Lambertian material
   * @return Albedo color normalized by pi
   */
  Color get_color() const override;

  float pdf(const Vec3 &wi, const Vec3 &wo,
                        const Vec3 &n) const override;

  /// Diffuse BRDF f = albedo(u,v)/pi, sampling the (possibly textured) albedo at
  /// the hit point so NEE lights textured surfaces (e.g. the checker floor)
  /// correctly rather than at a fixed UV.
  Color eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
             float u = 0.f, float v = 0.f) const override;


private:
  std::shared_ptr<const Texture> texture;
  std::shared_ptr<const Texture> normal_map = nullptr;
};

}  // namespace kestrel
