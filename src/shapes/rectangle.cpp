#include "rectangle.h"
#include "constants.h"
#include <cmath>

namespace kestrel {

Rectangle::Rectangle(const Material *material) : Shape(material) {
  // Mitsuba convention: the square spans [-1, 1] x [-1, 1] in the XY plane at
  // z=0; the toWorld transform then places/scales it.
  upper_left   = Vec3(-1.0f,  1.0f, 0.0f);
  upper_right  = Vec3( 1.0f,  1.0f, 0.0f);
  bottom_left  = Vec3(-1.0f, -1.0f, 0.0f);
  bottom_right = Vec3( 1.0f, -1.0f, 0.0f);
  this->compute_bounds();
}

bool Rectangle::hit(const Ray &ray, float t_min, float t_max,
                    HitRecord &rec) const {
  // Edge vectors derived from the stored corners so the test works for any
  // planar parallelogram orientation (XZ, XY, rotated, scaled, …).
  Vec3 p0 = upper_left;
  Vec3 e1 = upper_right - upper_left;   // top edge
  Vec3 e2 = bottom_left  - upper_left;  // left edge

  // Compute normal from cross product (accounts for flipped scales)
  Vec3 normal = Vec3::cross(e1, e2).normalized();

  float denom = Vec3::dot(normal, ray.direction);
  constexpr float eps = INTERSECTION_EPSILON;
  if (fabs(denom) < eps) {
    return false; // Ray is parallel to rectangle plane
  }

  float t = Vec3::dot(normal, p0 - ray.origin) / denom;
  if (t < t_min || t > t_max) {
    return false;
  }

  Vec3 hit_point = ray.at(t);

  // Project hit_point onto local rectangle axes and check bounds
  float len_e1 = e1.length();
  float len_e2 = e2.length();
  if (len_e1 < eps || len_e2 < eps)
    return false;

  Vec3 e1n = e1 / len_e1;
  Vec3 e2n = e2 / len_e2;

  float u = Vec3::dot(hit_point - p0, e1n);
  float v = Vec3::dot(hit_point - p0, e2n);

  if (u < -eps || u > len_e1 + eps)
    return false;
  if (v < -eps || v > len_e2 + eps)
    return false;

  rec.t = t;
  rec.point = hit_point;
  rec.set_face_normal(ray, normal);
  rec.u = u / len_e1;
  rec.v = v / len_e2;
  rec.material = this->material;
  return true;
}

void Rectangle::scale(const Vec3 &factor) {
  upper_left = upper_left * factor;
  upper_right = upper_right * factor;
  bottom_left = bottom_left * factor;
  bottom_right = bottom_right * factor;
  this->compute_bounds();
}

void Rectangle::translate(const Vec3 &offset) {
  upper_left = upper_left + offset;
  upper_right = upper_right + offset;
  bottom_left = bottom_left + offset;
  bottom_right = bottom_right + offset;
  _centroid += offset;
  this->compute_bounds();
}

void Rectangle::rotate(float rotate_angle, const Vec3 &rotate_axis) {
  // Convert angle to radians
  float angle_rad = rotate_angle * DEG_TO_RAD;

  // Create rotation matrix using Rodrigues' rotation formula
  Vec3 axis = rotate_axis.normalized();
  float cos_theta = cos(angle_rad);
  float sin_theta = sin(angle_rad);
  float one_minus_cos = 1.0f - cos_theta;

  float x = axis.x;
  float y = axis.y;
  float z = axis.z;

  // Rotation matrix components
  float r00 = cos_theta + x * x * one_minus_cos;
  float r01 = x * y * one_minus_cos - z * sin_theta;
  float r02 = x * z * one_minus_cos + y * sin_theta;

  float r10 = y * x * one_minus_cos + z * sin_theta;
  float r11 = cos_theta + y * y * one_minus_cos;
  float r12 = y * z * one_minus_cos - x * sin_theta;

  float r20 = z * x * one_minus_cos - y * sin_theta;
  float r21 = z * y * one_minus_cos + x * sin_theta;
  float r22 = cos_theta + z * z * one_minus_cos;

  // Rotate each corner point
  auto rotate_point = [&](const Vec3 &p) -> Vec3 {
    return Vec3(
        r00 * p.x + r01 * p.y + r02 * p.z,
        r10 * p.x + r11 * p.y + r12 * p.z,
        r20 * p.x + r21 * p.y + r22 * p.z
    );
  };

  upper_left = rotate_point(upper_left);
  upper_right = rotate_point(upper_right);
  bottom_left = rotate_point(bottom_left);
  bottom_right = rotate_point(bottom_right);
  
  _centroid = rotate_point(_centroid);

  this->compute_bounds();
}

bool Rectangle::sample_light(const Point3 &from, PCG32 &rng,
                             Vec3 &sample_point, float &pdf_sa) const {
  // Uniform sample over the parallelogram spanned by the two edges from
  // upper_left (the same parametrization Rectangle::hit uses).
  Vec3  e1 = upper_right - upper_left;
  Vec3  e2 = bottom_left - upper_left;
  float u  = rng.next_float(), v = rng.next_float();
  sample_point = upper_left + e1 * u + e2 * v;

  Vec3  ng   = Vec3::cross(e1, e2);
  float area = ng.length();                 // |e1 x e2| = parallelogram area
  if (area < INTERSECTION_EPSILON) return false;
  Vec3  n    = ng / area;

  Vec3  to      = sample_point - from;
  float dist_sq = to.length_squared();
  if (dist_sq < INTERSECTION_EPSILON) return false;
  Vec3  dir     = to / std::sqrt(dist_sq);

  // Two-sided emitter: it faces whichever side the receiver is on.
  float cos_light = std::fabs(Vec3::dot(n, dir));
  if (cos_light < INTERSECTION_EPSILON) return false;

  pdf_sa = dist_sq / (area * cos_light);     // area-measure -> solid-angle
  return true;
}

float Rectangle::light_pdf(const Point3 &from, const Vec3 &dir) const {
  HitRecord rec;
  Ray ray(from, dir);
  if (!hit(ray, RAY_EPSILON, RAY_TMAX, rec)) return 0.0f;

  Vec3  e1 = upper_right - upper_left;
  Vec3  e2 = bottom_left - upper_left;
  float area = Vec3::cross(e1, e2).length();
  if (area < INTERSECTION_EPSILON) return 0.0f;
  Vec3  n = Vec3::cross(e1, e2) / area;

  float cos_light = std::fabs(Vec3::dot(n, dir));
  if (cos_light < INTERSECTION_EPSILON) return 0.0f;

  float dist_sq = (rec.point - from).length_squared();
  return dist_sq / (area * cos_light);
}

}  // namespace kestrel
