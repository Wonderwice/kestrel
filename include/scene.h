/**
 * @file scene.h
 * @brief Scene representation with objects and camera
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "envmap.h"
#include "material.h"
#include "camera.h"
#include "light.h"
#include "shape.h"
#include "texture.h"
#include "accel/bvh.h"
#include <vector>
#include <memory>

/**
 * @brief A renderable 3D scene: owns its geometry, materials, textures, camera
 * and environment map, and accelerates ray queries with a BVH.
 *
 * Built once by the parser (the `add_*` / `set_*` builder API), finalised with
 * build(), then queried read-only by the integrators.
 */
namespace kestrel {

class Scene {
public:
  Scene() = default;

  // ---- Builder API (used by the scene parser) ----------------------------

  /// Add an object to the scene, taking ownership.
  void add_object(const Shape *obj) { objects_.emplace_back(obj); }
  /// Add a BSDF to the scene, taking ownership.
  void add_bsdf(const Material *bsdf) { bsdfs_.emplace_back(bsdf); }
  /// Register a shared texture referenced by id from one or more BSDFs.
  void add_texture(std::shared_ptr<const Texture> tex) { textures_.push_back(std::move(tex)); }
  /// Add a point light to the scene.
  void add_light(const Light &light) { lights_.push_back(light); }

  void set_camera(std::unique_ptr<const Camera> cam) { camera_ = std::move(cam); }
  void set_env_map(std::unique_ptr<const EnvMap> env) { env_map_ = std::move(env); }
  void set_background_color(const Color &c) { background_color_ = c; }
  void set_sample_count(int n) { sample_count_ = n; }

  /**
   * @brief Build the BVH over all objects and cache the emissive subset.
   * Call once after all objects have been added.
   */
  void build() {
    std::vector<const Shape *> prims;
    prims.reserve(objects_.size());
    for (const auto &obj : objects_) prims.push_back(obj.get());
    accel_.build(std::move(prims));

    // Collect emissive shapes for MIS light sampling, in the BVH's primitive
    // order so light selection by index is independent of insertion order.
    emissives_.clear();
    for (const Shape *obj : accel_.primitives()) {
      if (obj->material) {
        Color Le = obj->material->emitted();
        if (Le.x > 0 || Le.y > 0 || Le.z > 0)
          emissives_.push_back(obj);
      }
    }
  }

  // ---- Query API (used by the integrators / renderer) --------------------

  const Camera &camera() const { return *camera_; }
  const std::vector<Light> &lights() const { return lights_; }
  const std::vector<const Shape *> &emissives() const { return emissives_; }
  const EnvMap *env_map() const { return env_map_.get(); }
  const Color &background_color() const { return background_color_; }
  int sample_count() const { return sample_count_; }

  /// Closest-hit query against the scene (fills `rec`, including material).
  bool hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const;

  /// Any-hit occlusion query for shadow / NEE rays (stops at the first blocker).
  bool occluded(const Ray &ray, float t_min, float t_max) const;

  /// Combined solid-angle PDF of all emissive shapes for a direction.
  float emissive_pdf(const Point3 &from, const Vec3 &dir) const {
    if (emissives_.empty()) return 0.0f;
    float total = 0.0f;
    for (const auto *e : emissives_)
      total += e->light_pdf(from, dir);
    return total / static_cast<float>(emissives_.size());
  }

private:
  std::unique_ptr<const Camera> camera_;                ///< Camera used for rendering
  std::vector<std::unique_ptr<const Shape>> objects_;   ///< Objects in the scene (owned)
  std::vector<Light> lights_;                           ///< Point lights in the scene
  std::vector<std::unique_ptr<const Material>> bsdfs_;  ///< BSDFs in the scene (owned)
  std::vector<std::shared_ptr<const Texture>> textures_; ///< Shared textures referenced by id
  std::vector<const Shape *> emissives_;   ///< Non-owning subset of objects with emissive material
  std::unique_ptr<const EnvMap> env_map_;  ///< Optional HDR environment map
  Color background_color_{0.5f, 0.5f, 0.5f}; ///< Radiance for rays that miss all geometry
  int sample_count_ = 1;                   ///< Samples per pixel (from <sampler>)
  BVH accel_;                              ///< Bounding-volume hierarchy over `objects_`
};

}  // namespace kestrel
