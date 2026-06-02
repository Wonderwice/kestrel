#include "accel/bvh.h"

#include <algorithm>
#include <limits>

namespace kestrel {

void BVH::build(std::vector<const Shape *> prims) {
  prims_ = std::move(prims);
  nodes_.clear();
  if (prims_.empty())
    return;
  // A binary tree with N leaves of >=1 primitive has <= 2N-1 nodes; reserve up
  // front so the depth-first emission never reallocates mid-build (which would
  // be fine since we index by position, but reserving avoids the churn).
  nodes_.reserve(2 * prims_.size());
  build_node(0, static_cast<int>(prims_.size()));
}

int BVH::build_node(int start, int end) {
  // Claim this node's slot now so children (emitted by the recursive calls
  // below) land at higher indices; the left child becomes this index + 1.
  int node_idx = static_cast<int>(nodes_.size());
  nodes_.emplace_back();

  // Bounds of all primitives in [start, end).
  AABB box;
  for (int i = start; i < end; i++)
    box.expand(prims_[i]->bounds());
  box.pad();  // avoid zero-thickness boxes (coplanar leaves) being culled

  int count = end - start;
  constexpr int kLeafThreshold = 4;
  if (count <= kLeafThreshold) {
    nodes_[node_idx].bounds = box;
    nodes_[node_idx].offset = start;
    nodes_[node_idx].count = count;
    return node_idx;
  }

  // Split along the longest axis of the centroid bounds (the extent that
  // actually separates primitives), binning by centroid for a surface-area
  // heuristic (SAH) split.
  AABB centroid_box;
  for (int i = start; i < end; i++)
    centroid_box.expand(prims_[i]->centroid());
  int axis = centroid_box.longest_axis();
  float c_min = centroid_box.min[axis];
  float c_max = centroid_box.max[axis];

  int mid;
  if (c_max - c_min < 1e-12f) {
    // Degenerate: all centroids coincide on this axis. Fall back to a median
    // split so recursion still terminates and stays balanced.
    std::sort(prims_.begin() + start, prims_.begin() + end,
              [axis](const Shape *a, const Shape *b) {
                return a->centroid()[axis] < b->centroid()[axis];
              });
    mid = start + count / 2;
  } else {
    constexpr int kBins = 12;
    struct Bin { AABB bounds; int count = 0; };
    Bin bins[kBins];
    float scale = kBins / (c_max - c_min);
    auto bin_of = [&](const Shape *s) {
      int b = static_cast<int>((s->centroid()[axis] - c_min) * scale);
      return b < 0 ? 0 : (b >= kBins ? kBins - 1 : b);
    };
    for (int i = start; i < end; i++) {
      Bin &bin = bins[bin_of(prims_[i])];
      bin.bounds.expand(prims_[i]->bounds());
      bin.count++;
    }

    // Sweep to accumulate left/right bounds and counts for each of the kBins-1
    // candidate split planes, then pick the minimum-SAH-cost plane.
    AABB left_box[kBins - 1];
    int  left_count[kBins - 1];
    AABB acc; int acc_count = 0;
    for (int i = 0; i < kBins - 1; i++) {
      acc.expand(bins[i].bounds);
      acc_count += bins[i].count;
      left_box[i] = acc;
      left_count[i] = acc_count;
    }
    AABB right_acc; int right_acc_count = 0;
    float best_cost = std::numeric_limits<float>::infinity();
    int best_split = -1;  // bins [0..best_split] go left
    for (int i = kBins - 2; i >= 0; i--) {
      right_acc.expand(bins[i + 1].bounds);
      right_acc_count += bins[i + 1].count;
      if (left_count[i] == 0 || right_acc_count == 0) continue;
      float cost = left_count[i]  * left_box[i].surface_area() +
                   right_acc_count * right_acc.surface_area();
      if (cost < best_cost) { best_cost = cost; best_split = i; }
    }

    // SAH found no useful split (e.g. everything piled in one bin): median split.
    if (best_split < 0) {
      std::sort(prims_.begin() + start, prims_.begin() + end,
                [axis](const Shape *a, const Shape *b) {
                  return a->centroid()[axis] < b->centroid()[axis];
                });
      mid = start + count / 2;
    } else {
      auto mid_it = std::partition(
          prims_.begin() + start, prims_.begin() + end,
          [&](const Shape *s) { return bin_of(s) <= best_split; });
      mid = static_cast<int>(mid_it - prims_.begin());
      // Guard against a degenerate partition (shouldn't happen given the
      // count checks above, but keep recursion well-defined).
      if (mid == start || mid == end) mid = start + count / 2;
    }
  }

  // Emit the left subtree first so it occupies node_idx + 1, then the right.
  // Reload nodes_[node_idx] only after recursion: the emplace_backs above may
  // reallocate the vector, so we must not hold a reference across them.
  build_node(start, mid);
  int right_idx = build_node(mid, end);
  nodes_[node_idx].bounds = box;
  nodes_[node_idx].offset = right_idx;  // right child; left child = node_idx + 1
  nodes_[node_idx].count = 0;           // interior
  return node_idx;
}

// Depth of the traversal stack. Tree depth is bounded well under this for any
// realistic primitive count (each split keeps >=1 primitive per side), and 64
// is the conventional safe ceiling for a stack-based BVH walk.
static constexpr int kStackSize = 64;

bool BVH::hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const {
  if (nodes_.empty())
    return false;

  bool hit_anything = false;
  float closest_so_far = t_max;

  int stack[kStackSize];
  int sp = 0;
  stack[sp++] = 0;  // root

  while (sp > 0) {
    int ni = stack[--sp];
    const FlatNode &node = nodes_[ni];
    // Re-test against the (possibly tightened) closest hit so far so a node
    // pushed before a nearer hit was found gets culled here.
    if (!node.bounds.intersect(ray, t_min, closest_so_far))
      continue;

    if (node.count > 0) {  // leaf
      for (int i = node.offset; i < node.offset + node.count; i++) {
        if (prims_[i]->hit(ray, t_min, closest_so_far, rec)) {
          hit_anything = true;
          closest_so_far = rec.t;
          rec.material = prims_[i]->material;
        }
      }
      continue;
    }

    // Interior: left child is the next node, right child is `offset`. Visit the
    // nearer child first by pushing the farther one first (it's popped last).
    int c0 = ni + 1;          // left child
    int c1 = node.offset;     // right child
    float t0, t1;
    bool h0 = nodes_[c0].bounds.intersect(ray, t_min, closest_so_far, t0);
    bool h1 = nodes_[c1].bounds.intersect(ray, t_min, closest_so_far, t1);
    if (h0 && h1) {
      if (t0 <= t1) { stack[sp++] = c1; stack[sp++] = c0; }
      else          { stack[sp++] = c0; stack[sp++] = c1; }
    } else if (h0) {
      stack[sp++] = c0;
    } else if (h1) {
      stack[sp++] = c1;
    }
  }
  return hit_anything;
}

bool BVH::occluded(const Ray &ray, float t_min, float t_max) const {
  if (nodes_.empty())
    return false;

  int stack[kStackSize];
  int sp = 0;
  stack[sp++] = 0;  // root

  while (sp > 0) {
    int ni = stack[--sp];
    const FlatNode &node = nodes_[ni];
    if (!node.bounds.intersect(ray, t_min, t_max))
      continue;

    if (node.count > 0) {  // leaf
      for (int i = node.offset; i < node.offset + node.count; i++)
        if (prims_[i]->occluded(ray, t_min, t_max))
          return true;
      continue;
    }

    // Any-hit: no need to order children, just visit both.
    stack[sp++] = ni + 1;        // left
    stack[sp++] = node.offset;   // right
  }
  return false;
}

}  // namespace kestrel
