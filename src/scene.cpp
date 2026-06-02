#include "scene.h"

namespace kestrel {

bool Scene::hit(const Ray &ray, float t_min, float t_max, HitRecord &rec) const {
  return accel_.hit(ray, t_min, t_max, rec);
}

bool Scene::occluded(const Ray &ray, float t_min, float t_max) const {
  return accel_.occluded(ray, t_min, t_max);
}

}  // namespace kestrel
