/**
 * @file material.h
 * @brief Material interface for ray tracing
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "kestrel.h"
#include "ray.h"
#include "vec3.h"

namespace kestrel {

class HitRecord;
/**
 * @class Material
 * @brief Material base class for ray tracing
 */
class Material {
public:
  /**
   * @brief Virtual destructor
   */
  virtual ~Material() = default;

  /**
   * @brief Scatter an incoming ray
   * @param incoming Incoming ray direction
   * @param rec Hit record with surface normal
   * @param attenuation Output color attenuation (BRDF * cos / pdf)
   * @param scattered Output scattered ray
   * @param pdf_out Output solid-angle pdf of the sampled direction; set to 0 to
   *                signal a delta/specular event (no MIS, no NEE double-count)
   * @return True if scattering occurred
   */
  virtual bool scatter(const Ray &incoming, const HitRecord &rec,
                                   Color &attenuation, Ray &scattered,
                                   float &pdf_out, PCG32 &rng) const = 0;

  /**
   * @brief Get the color of the material
   * @return Material color
   */
  virtual Color get_color() const = 0;

  /**
   * @brief Evaluate the directional BRDF f(wi, wo) at a hit point.
   *
   * Used by next-event estimation (NEE) to weight light samples. (u,v) are the
   * surface texture coordinates at the hit point so textured BRDFs can look up
   * their reflectance there — NEE samples many surface points, so evaluating the
   * texture at a fixed UV would mis-light textured materials (e.g. a checker
   * floor). The default returns get_color() for untextured/constant materials.
   * @param wi Normalized incoming direction (toward the viewer)
   * @param wo Normalized outgoing direction (toward the light / next bounce)
   * @param n  Surface normal at the hit point
   * @param u,v Texture coordinates at the hit point
   * @return BRDF value f (including any 1/pi normalisation)
   */
  virtual Color eval(const Vec3 & /*wi*/, const Vec3 & /*wo*/, const Vec3 & /*n*/,
                     float /*u*/ = 0.f, float /*v*/ = 0.f) const { return get_color(); }

  /**
   * @brief Light emitted from this material (zero for non-emissive materials)
   * @return Emitted radiance
   */
  virtual Color emitted() const { return Color(0, 0, 0); }

  /**
   * @brief Evaluate the PDF of the scattered direction (for MIS).
   * @param wi Normalized incoming direction (toward surface)
   * @param wo Normalized outgoing (scattered) direction
   * @param n  Surface normal at the hit point
   * @return PDF value (solid angle measure), 0 for delta distributions
   */
  virtual float pdf(const Vec3 & /*wi*/, const Vec3 & /*wo*/,
                                const Vec3 & /*n*/) const { return 0.0f; }

  /**
   * @brief Whether this is a purely specular material whose reflection is
   * importance-sampled by its own lobe, so next-event estimation should be
   * skipped for it (mirror, glass, microfacet metal/glass).
   */
  virtual bool is_specular() const { return false; }
};

}  // namespace kestrel
