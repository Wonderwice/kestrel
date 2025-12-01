#include "sphere.h"

HOST_DEVICE bool Sphere::hit(const Ray &ray, float t_min, float t_max,
                 HitRecord &rec) const {
  // Solve quadratic equation for ray-sphere intersection
  Vec3 oc = ray.origin - center;
  float a = ray.direction.length_squared();
  float half_b = Vec3::dot(oc, ray.direction);
  float c = oc.length_squared() - radius * radius;

  // Check discriminant to determine if intersection exists
  float discriminant = half_b * half_b - a * c;
  if (discriminant < 0)
    return false;

  float sqrtd = std::sqrt(discriminant);

  // Find the nearest root that lies in the acceptable range [t_min, t_max]
  float root = (-half_b - sqrtd) / a;
  if (root < t_min || t_max < root) {
    // Try the other root
    root = (-half_b + sqrtd) / a;
    if (root < t_min || t_max < root)
      return false;
  }

  // Fill hit record with intersection information
  rec.t = root;
  rec.point = ray.at(rec.t);
  Vec3 outward_normal = (rec.point - center) / radius;
  rec.set_face_normal(ray, outward_normal);

  return true;
}