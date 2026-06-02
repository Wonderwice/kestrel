/**
 * @file bvh.h
 * @brief Bounding-volume hierarchy over a set of primitives (Shapes).
 * @author Alexei Czornyj
 * @date 2026
 *
 * A single reusable acceleration structure shared by the Scene (over all scene
 * objects) and Mesh (over its triangles), replacing the two hand-rolled copies
 * that previously lived in scene.h and mesh.cpp. Nodes are stored contiguously
 * in a flat, depth-first array and traversed with an explicit stack (no pointer
 * chasing or per-node allocation). Primitives are referenced (not owned); the
 * caller keeps them alive.
 */

#pragma once

#include "accel/aabb.h"
#include "shape.h"
#include <vector>

namespace kestrel {

class BVH {
public:
  BVH() = default;

  /// Build the hierarchy over `prims`. The list is reordered internally and
  /// retained by reference; the pointed-to Shapes must outlive the BVH.
  void build(std::vector<const Shape *> prims);

  /// True if no primitives were built into the hierarchy.
  bool empty() const { return nodes_.empty(); }

  /// Closest-hit query: fills `rec` (including material) with the nearest hit.
  bool hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const;

  /// Any-hit query for shadow / NEE rays: stops at the first blocker.
  bool occluded(const Ray &ray, float t_min, float t_max) const;

  /// Primitives in their post-build (leaf-contiguous) order.
  const std::vector<const Shape *> &primitives() const { return prims_; }

private:
  /// A node in the flattened, depth-first hierarchy. Nodes live contiguously in
  /// `nodes_` (cache-friendly, no per-node heap allocation or pointer chasing).
  /// For an interior node the LEFT child is implicitly the next node in the
  /// array (this index + 1) and `offset` holds the RIGHT child's index; for a
  /// leaf `offset` is the first primitive index. `count` is 0 for interior nodes
  /// and the primitive count for leaves. 32 bytes, two per cache line.
  struct FlatNode {
    AABB bounds;     ///< 24 bytes (two Point3)
    int  offset = 0; ///< interior: right-child index; leaf: first primitive index
    int  count = 0;  ///< 0 = interior; >0 = leaf primitive count
  };

  /// Recursively build the subtree over prims_[start, end), emitting nodes into
  /// `nodes_` in depth-first order. Returns the index of the emitted node.
  int build_node(int start, int end);

  std::vector<const Shape *> prims_;
  std::vector<FlatNode> nodes_;  ///< node 0 is the root (empty if no prims)
};

}  // namespace kestrel
