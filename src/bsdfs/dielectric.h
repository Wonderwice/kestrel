/**
 * @file dielectric.h
 * @brief Dielectric (glass/water) BSDF using Fresnel equations
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class Dielectric
 * @brief Fresnel dielectric: combines specular reflection and refraction.
 *
 * Uses Schlick's approximation for the Fresnel reflectance and falls back to
 * total internal reflection when the transmitted angle would be imaginary.
 * The scatter attenuation is always (1,1,1) — dielectrics don't absorb.
 */
namespace kestrel {

class Dielectric : public Material {
public:
  Dielectric() : ior(1.5f) {}
  Dielectric(float ior) : ior(ior) {}

  ~Dielectric() override {}

  bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered,
                           float &pdf_out, PCG32 &rng) const override;

  Color get_color() const override { return Color(1, 1, 1); }

  bool is_specular() const override { return true; }

private:
  float ior; ///< Interior index of refraction (exterior assumed to be 1.0)

  static float schlick(float cos_theta, float ref_idx) {
    float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 *= r0;
    return r0 + (1.0f - r0) * std::pow(1.0f - cos_theta, 5.0f);
  }
};

}  // namespace kestrel
