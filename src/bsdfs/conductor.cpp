#include "conductor.h"
#include "constants.h"

namespace kestrel {

bool Conductor::scatter(const Ray &incoming, const HitRecord &rec,
                        Color &attenuation, Ray &scattered,
                        float &pdf_out, PCG32 &) const {
  Vec3 in_dir = incoming.direction;
  Vec3 scatter_direction = in_dir - 2.0f * Vec3::dot(in_dir, rec.normal) * rec.normal;
  // Offset the origin slightly along the normal to avoid self-intersection
  Point3 origin = rec.point + rec.normal * RAY_EPSILON;
  scattered = Ray(origin, scatter_direction);
  attenuation = albedo;
  pdf_out = 0.0f; // perfect specular: delta distribution
  return true;
}

Color Conductor::get_color() const { return albedo; }

}  // namespace kestrel
