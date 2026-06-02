#include "dielectric.h"
#include "constants.h"
#include "pcg32.h"

namespace kestrel {

bool Dielectric::scatter(const Ray &incoming, const HitRecord &rec,
                         Color &attenuation, Ray &scattered,
                         float &pdf_out, PCG32 &rng) const {
  attenuation = Color(1, 1, 1);
  pdf_out = 0.0f; // specular reflection/refraction: delta distribution
  float ratio = rec.front_face ? (1.0f / ior) : ior;

  Vec3 unit_dir = incoming.direction.normalized();
  float cos_theta = std::min(-Vec3::dot(unit_dir, rec.normal), 1.0f);
  float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

  bool must_reflect = ratio * sin_theta > 1.0f;
  Vec3 direction;

  if (must_reflect || schlick(cos_theta, ratio) > rng.next_float()) {
    direction = reflect(unit_dir, rec.normal);
  } else {
    direction = refract(unit_dir, rec.normal, ratio);
  }

  // Offset the origin onto whichever side the scattered ray actually leaves on
  // (reflection stays on the incident side, refraction crosses to the other) —
  // keyed on the geometric normal so it is robust on meshes. Keying the offset
  // on front_face alone pushes reflected rays to the wrong side and lets them
  // self-intersect.
  float side = Vec3::dot(direction, rec.geo_normal) < 0.0f ? -1.0f : 1.0f;
  scattered = Ray(rec.point + side * rec.geo_normal * RAY_EPSILON, direction);
  return true;
}

}  // namespace kestrel
