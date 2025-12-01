#include "light.h"

Vec3 Light::get_intensity() const {
  return intensity;
}

Vec3 Light::sample_direction(const Vec3 &point) const {
  // Sample direction from point to light position
  return (position - point).normalized();
}