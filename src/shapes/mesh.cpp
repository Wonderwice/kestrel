#include "mesh.h"
#include <vector>

namespace kestrel {

Mesh::~Mesh() = default;

bool Mesh::hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const {
  return bvh_.hit(ray, t_min, t_max, rec);
}

bool Mesh::occluded(const Ray &ray, float t_min, float t_max) const {
  return bvh_.occluded(ray, t_min, t_max);
}

void Mesh::scale(const Vec3 &factor) {
  for (auto &tri : triangles)
    tri.scale(factor);
  compute_bounds();
}

void Mesh::translate(const Vec3 &offset) {
  for (auto &tri : triangles)
    tri.translate(offset);
  compute_bounds();
}

void Mesh::rotate(float angle_degrees, const Vec3 &axis) {
  for (auto &tri : triangles)
    tri.rotate(angle_degrees, axis);
  compute_bounds();
}

void Mesh::transform(const Mat4 &m) {
  for (auto &tri : triangles)
    tri.transform(m);
  compute_bounds();
}

void Mesh::build_bvh() {
  if (triangles.empty()) {
    bvh_.build({});
    return;
  }
  // Build the hierarchy over the triangles, then reorder `triangles` into the
  // BVH's leaf order. The scene loader explodes a mesh into individual scene
  // primitives in this order, so exposing the spatially-sorted order keeps the
  // scene's primitive sequence (and hence ray-intersection tie-breaking)
  // deterministic and cache-coherent.
  auto gather = [this](std::vector<const Shape *> &out) {
    out.clear();
    out.reserve(triangles.size());
    for (const auto &tri : triangles) out.push_back(&tri);
  };
  std::vector<const Shape *> prims;
  gather(prims);
  bvh_.build(std::move(prims));

  std::vector<Triangle> reordered;
  reordered.reserve(triangles.size());
  for (const Shape *p : bvh_.primitives())
    reordered.push_back(*static_cast<const Triangle *>(p));
  triangles = std::move(reordered);

  // Rebuild so the leaf pointers reference the reordered storage.
  gather(prims);
  bvh_.build(std::move(prims));
}

void Mesh::compute_bounds() {
  _bounds = AABB();
  for (const auto &tri : triangles) {
    _bounds.expand(tri.bounds().min);
    _bounds.expand(tri.bounds().max);
  }
  _centroid = (_bounds.min + _bounds.max) * 0.5f;
  build_bvh();
}

}  // namespace kestrel
