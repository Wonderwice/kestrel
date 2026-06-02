#include "triangle.h"
#include "constants.h"
#include <cmath>

namespace kestrel {

bool Triangle::hit(const Ray &ray, float t_min, float t_max,
                   HitRecord &rec) const {
  // Möller–Trumbore intersection algorithm
  constexpr float eps = INTERSECTION_EPSILON;
  float t = 0.0f;
  const Vec3 &edge1 = e1_;  // cached v1 - v0
  const Vec3 &edge2 = e2_;  // cached v2 - v0
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

  // Barycentric coords: u->v1, v->v2, w->v0
  float bw = 1.0f - u - v;
  // Shading normal (interpolated) and geometric normal (from winding). Decide
  // front/back facing from the GEOMETRIC normal: deriving it from the shading
  // normal (a naive self-flip) mis-flips near grazing angles on smooth meshes and
  // darkens silhouettes. Orient the geometric normal to agree with the authored
  // shading normals, then face the shading normal toward the ray.
  Vec3 ns = (n0 * bw + n1 * u + n2 * v).normalized();
  Vec3 ng = Vec3::cross(edge1, edge2);
  if (Vec3::dot(ng, n0 + n1 + n2) < 0.0f) ng = -1.0f * ng;
  rec.front_face = Vec3::dot(ray.direction, ng) < 0.0f;
  if (Vec3::dot(ns, ng) < 0.0f) ns = -1.0f * ns;
  rec.normal = rec.front_face ? ns : -1.0f * ns;
  // Geometric face normal, oriented against the ray like the shading normal.
  // Shadow/secondary rays offset along this to avoid self-intersection acne
  // that the shading normal produces on smooth meshes (see HitRecord).
  Vec3 ngn = ng.normalized();
  rec.geo_normal = rec.front_face ? ngn : -1.0f * ngn;
  rec.u = tu0 * bw + tu1 * u + tu2 * v;
  rec.v = tv0 * bw + tv1 * u + tv2 * v;

  // Compute tangent from UV edges (used for normal mapping)
  const Vec3 &e1 = e1_, &e2 = e2_;
  float du1 = tu1 - tu0, dv1 = tv1 - tv0;
  float du2 = tu2 - tu0, dv2 = tv2 - tv0;
  float uv_det = du1 * dv2 - du2 * dv1;
  if (std::abs(uv_det) > 1e-8f)
    rec.tangent = (e1 * dv2 - e2 * dv1) * (1.0f / uv_det);
  else
    rec.tangent = Vec3::cross(rec.normal, Vec3(0,1,0)).length_squared() > 1e-6f
                  ? Vec3::cross(rec.normal, Vec3(0,1,0)).normalized()
                  : Vec3::cross(rec.normal, Vec3(1,0,0)).normalized();

  rec.material = material;
  return true;
}

bool Triangle::occluded(const Ray &ray, float t_min, float t_max) const {
  // Möller–Trumbore, but we only care whether a hit exists in range — no
  // barycentric shading data, normals, UVs or tangents are computed.
  constexpr float eps = INTERSECTION_EPSILON;
  const Vec3 &edge1 = e1_;  // cached v1 - v0
  const Vec3 &edge2 = e2_;  // cached v2 - v0
  Vec3 h = Vec3::cross(ray.direction, edge2);
  float det = Vec3::dot(edge1, h);

  if (det > -eps && det < eps)
    return false; // Ray is parallel to triangle
  float inv_det = 1.0f / det;
  Vec3 s = ray.origin - v0;
  float u = inv_det * Vec3::dot(s, h);
  Vec3 s_cross_e1 = Vec3::cross(s, edge1);
  if ((u < 0 && std::abs(u) > eps) || (u > 1 && std::abs(u - 1) > eps))
    return false;
  float v = inv_det * Vec3::dot(ray.direction, s_cross_e1);
  if ((v < 0 && std::abs(v) > eps) || (u + v > 1 && std::abs(u + v - 1) > eps))
    return false;

  float t = inv_det * Vec3::dot(edge2, s_cross_e1);
  return t >= t_min && t <= t_max;
}

void Triangle::scale(const Vec3 &factor) {
  v0 = v0 * factor;
  v1 = v1 * factor;
  v2 = v2 * factor;
  Vec3 inv(1.0f / factor.x, 1.0f / factor.y, 1.0f / factor.z);
  n0 = (n0 * inv).normalized();
  n1 = (n1 * inv).normalized();
  n2 = (n2 * inv).normalized();
  compute_bounds();
}

void Triangle::translate(const Vec3 &offset) {
  v0 = v0 + offset;
  v1 = v1 + offset;
  v2 = v2 + offset;
  compute_bounds();
}

void Triangle::rotate(float angle_degrees, const Vec3 &axis) {
  Vec3 normalized_axis = axis.normalized();
  float angle_radians = angle_degrees * (DEG_TO_RAD);
  float cos_theta = cos(angle_radians);
  float sin_theta = sin(angle_radians);

  Vec3 v0_rotated = v0 * cos_theta +
                   Vec3::cross(normalized_axis, v0) * sin_theta +
                   normalized_axis * Vec3::dot(normalized_axis, v0) * (1 - cos_theta);
  v0 = Point3(v0_rotated.x, v0_rotated.y, v0_rotated.z);

  Vec3 v1_rotated = v1 * cos_theta +
                   Vec3::cross(normalized_axis, v1) * sin_theta +
                   normalized_axis * Vec3::dot(normalized_axis, v1) * (1 - cos_theta);
  v1 = Point3(v1_rotated.x, v1_rotated.y, v1_rotated.z);

  Vec3 v2_rotated = v2 * cos_theta +
                   Vec3::cross(normalized_axis, v2) * sin_theta +
                   normalized_axis * Vec3::dot(normalized_axis, v2) * (1 - cos_theta);
  v2 = Point3(v2_rotated.x, v2_rotated.y, v2_rotated.z);

  n0 = (n0 * cos_theta + Vec3::cross(normalized_axis, n0) * sin_theta +
        normalized_axis * Vec3::dot(normalized_axis, n0) * (1 - cos_theta)).normalized();
  n1 = (n1 * cos_theta + Vec3::cross(normalized_axis, n1) * sin_theta +
        normalized_axis * Vec3::dot(normalized_axis, n1) * (1 - cos_theta)).normalized();
  n2 = (n2 * cos_theta + Vec3::cross(normalized_axis, n2) * sin_theta +
        normalized_axis * Vec3::dot(normalized_axis, n2) * (1 - cos_theta)).normalized();

  compute_bounds();
}

void Triangle::transform(const Mat4 &m) {
  v0 = m.transform_point(v0);
  v1 = m.transform_point(v1);
  v2 = m.transform_point(v2);

  // Shading normals transform by the inverse-transpose of the upper-left 3x3.
  n0 = m.transform_normal(n0);
  n1 = m.transform_normal(n1);
  n2 = m.transform_normal(n2);

  compute_bounds();
}

void Triangle::compute_bounds() {
  _bounds = AABB();
  _bounds.expand(v0);
  _bounds.expand(v1);
  _bounds.expand(v2);
  _centroid = (_bounds.min + _bounds.max) * 0.5f;
  // Cache the Möller–Trumbore edges so per-ray hit/occluded tests skip the two
  // vector subtractions; these are the hottest reads in the intersection loop.
  e1_ = v1 - v0;
  e2_ = v2 - v0;
}

bool Triangle::sample_light(const Point3 &from, PCG32 &rng,
                            Vec3 &sample_point, float &pdf_sa) const {
  // Uniform barycentric sample over the triangle surface.
  float u = rng.next_float(), v = rng.next_float();
  float su = std::sqrt(u);
  float b0 = 1.0f - su;
  float b1 = v * su;
  float b2 = 1.0f - b0 - b1;
  sample_point = v0 * b0 + v1 * b1 + v2 * b2;

  // Geometric normal and area from the edge cross product, oriented to agree
  // with the shading normals so it points in the emitting direction.
  Vec3  ng    = Vec3::cross(v1 - v0, v2 - v0);
  float two_a = ng.length();
  if (two_a < INTERSECTION_EPSILON) return false;
  float area  = 0.5f * two_a;
  Vec3  n      = ng / two_a;
  if (Vec3::dot(n, n0 + n1 + n2) < 0.0f) n = n * -1.0f;

  Vec3  to_sample = sample_point - from;
  float dist_sq   = to_sample.length_squared();
  if (dist_sq < INTERSECTION_EPSILON) return false;
  Vec3  dir       = to_sample / std::sqrt(dist_sq);

  // One-sided emission: the light only emits toward points its front face sees,
  // i.e. its normal must point back toward the shading point (dot(n, dir) < 0).
  float ndl = Vec3::dot(n, dir);
  if (ndl > -INTERSECTION_EPSILON) return false;
  float cos_light = -ndl;

  // Convert area-measure PDF (1/area) to solid-angle measure.
  pdf_sa = dist_sq / (area * cos_light);
  return true;
}

float Triangle::light_pdf(const Point3 &from, const Vec3 &dir) const {
  HitRecord rec;
  Ray ray(from, dir);
  if (!hit(ray, RAY_EPSILON, RAY_TMAX, rec)) return 0.0f;

  Vec3  ng    = Vec3::cross(v1 - v0, v2 - v0);
  float two_a = ng.length();
  if (two_a < INTERSECTION_EPSILON) return 0.0f;
  float area  = 0.5f * two_a;
  Vec3  n      = ng / two_a;
  if (Vec3::dot(n, n0 + n1 + n2) < 0.0f) n = n * -1.0f;

  // One-sided: a direction landing on the back face carries no emission.
  float ndl = Vec3::dot(n, dir);
  if (ndl > -INTERSECTION_EPSILON) return 0.0f;
  float cos_light = -ndl;

  float dist_sq = (rec.point - from).length_squared();
  return dist_sq / (area * cos_light);
}

}  // namespace kestrel
