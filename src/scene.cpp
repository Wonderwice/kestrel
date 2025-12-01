#include "scene.h"

HOST_DEVICE bool Scene::hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const {
  bool hit_anything = false;
  float closest_so_far = t_max;

  for (const auto &obj : objects) {
    if (obj.hit(ray, t_min, closest_so_far, rec)) {
      hit_anything = true;
      closest_so_far = rec.t;
      rec.material = obj.material;
    }
  }

  return hit_anything;
}