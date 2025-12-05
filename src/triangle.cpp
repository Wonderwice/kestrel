#include "triangle.h"
#include <cmath>

bool Triangle::hit(const Ray &ray, float t_min, float t_max,
                   HitRecord &rec) const {
  // Möller–Trumbore intersection algorithm
  float eps = 1e-8f;
  float t = 0.0f;
  Vec3 edge1 = v1 - v0;
  Vec3 edge2 = v2 - v0;
  Vec3 h = Vec3::cross(ray.direction, edge2);
  float det = Vec3::dot(edge1, h);

  if (det > -eps && det < eps)
    return false; // Ray is parallel to triangle
  float inv_det = 1.0f / det;
  Vec3 s = ray.origin - v0;
  float u = inv_det * Vec3::dot(s, h);
  Vec3 s_cross_e1 = Vec3::cross(s, edge1);
  if ((u < 0 && std::abs(u) > eps) || (u > 1 && std::abs(u - 1) > eps))
    return false; // Ray does not hit triangle
  float v = inv_det * Vec3::dot(ray.direction, s_cross_e1);

  if ((v < 0 && std::abs(v) > eps) || (u + v > 1 && std::abs(u + v - 1) > eps))
    return false; // Ray does not hit triangle

  t = inv_det * Vec3::dot(edge2, s_cross_e1);
  if (t < t_min || t > t_max)
    return false; // Intersection out of bounds

  rec.t = t;
  rec.point = ray.at(t);
  Vec3 outward_normal = Vec3::cross(edge1, edge2).normalized();
  rec.set_face_normal(ray, outward_normal);
  rec.material = material;
  return true;
}

void Triangle::scale(Vec3 factor) {
  v0 = v0 * factor.x;
  v1 = v1 * factor.y;
  v2 = v2 * factor.z;
}

void Triangle::translate(Vec3 offset) {
  v0 = v0 + offset;
  v1 = v1 + offset;
  v2 = v2 + offset;
}