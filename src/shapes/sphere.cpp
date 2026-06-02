#include "sphere.h"
#include "constants.h"
#include "pcg32.h"
#include <algorithm>
#include <cmath>

namespace kestrel {

bool Sphere::hit(const Ray &ray, float t_min, float t_max,
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

  // Spherical UV from outward normal.
  float theta = std::acos(std::clamp(outward_normal.y, -1.0f, 1.0f));
  float phi   = std::atan2(outward_normal.z, outward_normal.x);
  rec.u = 1.0f - (phi + PI) / TWO_PI;
  rec.v = 1.0f - theta / PI;

  return true;
}

void Sphere::compute_bounds(){
  this->_bounds = AABB();
  this->_bounds.expand(this->center + this->radius);
  this->_bounds.expand(this->center - this->radius);

  this->_centroid = this->center;
}

void Sphere::translate(const Vec3& offset) {
    this->center += offset;
    this->compute_bounds();
}

void Sphere::rotate(float rotate_degrees, const Vec3& rotate_axis) {}

void Sphere::scale(const Vec3& factor) {
    this->radius *= std::cbrt(factor.x * factor.y * factor.z);
    this->compute_bounds();
}

static void build_local_frame(const Vec3 &n, Vec3 &t, Vec3 &b) {
  Vec3 up = (std::abs(n.y) < 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
  t = Vec3::cross(up, n).normalized();
  b = Vec3::cross(n, t);
}

bool Sphere::sample_light(const Point3 &from, PCG32 &rng,
                           Vec3 &sample_point, float &pdf_sa) const {
  Vec3 to_center = center - from;
  float dist_sq  = to_center.length_squared();
  if (dist_sq <= radius * radius) return false; // inside sphere

  float cos_max = std::sqrt(1.0f - radius * radius / dist_sq);
  float solid_angle = TWO_PI * (1.0f - cos_max);

  // Sample uniformly over the cone subtended by the sphere.
  float u1 = rng.next_float(), u2 = rng.next_float();
  float cos_theta = 1.0f - u1 * (1.0f - cos_max);
  float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
  float phi = TWO_PI * u2;

  Vec3 axis = to_center.normalized();
  Vec3 t, b;
  build_local_frame(axis, t, b);

  Vec3 dir = (sin_theta * std::cos(phi)) * t
           + cos_theta * axis
           + (sin_theta * std::sin(phi)) * b;

  // Project to sphere surface to get sample_point
  // Approximate by intersecting the ray from 'from' in direction 'dir'
  float h = Vec3::dot(to_center, dir);
  float disc = h * h - (to_center.length_squared() - radius * radius);
  if (disc < 0.0f) {
    sample_point = center; // degenerate fallback
  } else {
    float t_hit = h - std::sqrt(disc);
    sample_point = from + t_hit * dir;
  }

  pdf_sa = 1.0f / solid_angle;
  return true;
}

float Sphere::light_pdf(const Point3 &from, const Vec3 &dir) const {
  Vec3 to_center = center - from;
  float dist_sq  = to_center.length_squared();
  if (dist_sq <= radius * radius) return 0.0f;
  float cos_max = std::sqrt(1.0f - radius * radius / dist_sq);
  float solid_angle = TWO_PI * (1.0f - cos_max);
  // Check if dir is inside the cone
  float cos_dir = Vec3::dot(dir.normalized(), to_center.normalized());
  if (cos_dir < cos_max) return 0.0f;
  return 1.0f / solid_angle;
}


}  // namespace kestrel
