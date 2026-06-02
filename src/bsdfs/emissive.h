/**
 * @file emissive.h
 * @brief Emissive (area light) material
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class Emissive
 * @brief Non-scattering material that emits radiance.
 *
 * Geometry with this material acts as an area light source. scatter()
 * returns false so the path terminates at the surface; radiance is
 * accumulated via emitted() in the integrator.
 */
namespace kestrel {

class Emissive : public Material {
public:
  Emissive() : emission(Color(1, 1, 1)) {}
  Emissive(Color emission) : emission(emission) {}

  ~Emissive() override {}

  bool scatter(const Ray &, const HitRecord &,
                           Color &, Ray &, float &pdf_out, PCG32 &) const override {
    pdf_out = 0.0f;
    return false;
  }

  Color get_color() const override { return Color(0, 0, 0); }

  Color emitted() const override { return emission; }

private:
  Color emission;
};

}  // namespace kestrel
