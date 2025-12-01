#include "bsdfs/conductor.h"

bool Conductor::scatter(const Ray &incoming, const HitRecord &rec,
                        Color &attenuation, Ray &scattered) const {
  Vec3 scatter_direction =
      incoming.direction -
      2.0f * Vec3::dot(incoming.direction, rec.normal) * rec.normal;
  scattered = Ray(rec.point, scatter_direction);
  attenuation = albedo;
  return true;
}

Color Conductor::get_color() const { return albedo; }