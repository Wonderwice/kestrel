#pragma once

#include "integrator.h"

namespace kestrel {

class PathIntegrator : public Integrator {
public:
  explicit PathIntegrator(int max_depth = 64) : max_depth(max_depth) {}
  Color integrate(const Ray &ray, const Scene &scene, PCG32 &rng) const override;

private:
  int max_depth;
};

}  // namespace kestrel
