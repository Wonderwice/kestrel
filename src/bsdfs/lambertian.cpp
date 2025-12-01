#include "bsdfs/lambertian.h"

bool Lambertian::scatter(const Ray &incoming, const HitRecord &rec,
                         Color &attenuation, Ray &scattered) const {
  Vec3 scatter_direction = rec.normal + Vec3::random_unit_vector();
  scattered = Ray(rec.point, scatter_direction);
  attenuation =
      albedo /
      3.14159265358979323846f; // Normalize by pi for energy conservation
  return true;
}

Color Lambertian::get_color() const { return albedo / 3.14159265358979323846f; }