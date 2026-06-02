#pragma once

#include "integrator.h"

/**
 * Direct-illumination only integrator.
 * Evaluates one-bounce lighting from point lights, area emitters, and the
 * environment map.  No recursive scattering or Russian roulette.
 *
 * Two single-strategy modes (no MIS), useful for the Veach comparison:
 *   - Light: next-event estimation only — sample a point on a light and
 *     connect with a shadow ray. Clean on rough/diffuse surfaces, noisy on
 *     near-specular ones (a sampled light point rarely lands in the lobe).
 *   - Bsdf:  BSDF sampling only — scatter one ray and keep its emission if it
 *     lands on a light. Clean on near-specular surfaces, noisy on rough ones
 *     (a wide BSDF sample rarely hits a small bright light).
 */
namespace kestrel {

class DirectIntegrator : public Integrator {
public:
  enum class Strategy { Light, Bsdf };
  explicit DirectIntegrator(Strategy s = Strategy::Light) : strategy_(s) {}
  Color integrate(const Ray &ray, const Scene &scene, PCG32 &rng) const override;

private:
  Strategy strategy_;
};

}  // namespace kestrel
