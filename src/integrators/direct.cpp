#include "direct_integrator.h"
#include "constants.h"
#include "scene.h"

namespace kestrel {

Color DirectIntegrator::integrate(const Ray &ray, const Scene &scene, PCG32 &rng) const {
  HitRecord rec;
  if (!scene.hit(ray, RAY_EPSILON, RAY_TMAX, rec)) {
    if (scene.env_map() && scene.env_map()->valid())
      return scene.env_map()->sample(ray.direction.normalized());
    return scene.background_color();
  }

  // Camera ray hit an emitter — return emission directly.
  Color Le = rec.material->emitted();
  if (Le.x > 0 || Le.y > 0 || Le.z > 0)
    return Le;

  // BSDF-sampling strategy: scatter one ray and keep its emission if it lands
  // on a light. No NEE, no MIS (implicit weight = 1). Works for specular too.
  if (strategy_ == Strategy::Bsdf) {
    Color attenuation;
    Ray   scattered;
    float pdf = 0.0f;
    if (!rec.material->scatter(ray, rec, attenuation, scattered, pdf, rng))
      return Color(0, 0, 0);

    HitRecord rec2;
    if (scene.hit(scattered, RAY_EPSILON, RAY_TMAX, rec2)) {
      Color Le2 = rec2.material->emitted();
      if ((Le2.x > 0 || Le2.y > 0 || Le2.z > 0) && rec2.front_face)
        return attenuation * Le2;  // BSDF ray landed on a light
      return Color(0, 0, 0);       // direct-only: non-emitter hit contributes nothing
    }
    if (scene.env_map() && scene.env_map()->valid())
      return attenuation * scene.env_map()->sample(scattered.direction.normalized());
    return attenuation * scene.background_color();
  }

  // Light-sampling (NEE) strategy below.
  // Specular surfaces have no diffuse direct contribution under NEE.
  if (rec.material->is_specular())
    return Color(0, 0, 0);

  Color direct(0, 0, 0);
  Vec3 wi = (ray.direction * -1.0f).normalized();

  // Point lights.
  for (const auto &light : scene.lights()) {
    Vec3  to_light  = light.position - rec.point;
    float dist      = to_light.length();
    Vec3  light_dir = to_light / dist;

    Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, light_dir);
    if (scene.occluded(shadow_ray, RAY_EPSILON, dist - RAY_EPSILON))
      continue;

    float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, light_dir));
    float dist_sq   = dist * dist + DISTANCE_EPSILON;
    direct += rec.material->eval(wi, light_dir, rec.normal, rec.u, rec.v)
              * cos_theta * light.get_intensity() / dist_sq;
  }

  // Area emitters — uniform NEE, one sample, no MIS (single strategy).
  if (!scene.emissives().empty()) {
    int  n   = static_cast<int>(scene.emissives().size());
    int  idx = std::min(static_cast<int>(rng.next_float() * n), n - 1);
    const Shape *light_shape = scene.emissives()[idx];

    Vec3  sample_pt;
    float light_pdf_sa;
    if (light_shape->sample_light(rec.point, rng, sample_pt, light_pdf_sa) &&
        light_pdf_sa > 0.0f) {
      light_pdf_sa /= n;
      Vec3  to_sample = sample_pt - rec.point;
      float dist_l    = to_sample.length();
      Vec3  dir_l     = to_sample / dist_l;

      Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, dir_l);
      if (!scene.occluded(shadow_ray, RAY_EPSILON, dist_l - RAY_EPSILON)) {
        float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, dir_l));
        Color Le_l      = light_shape->material->emitted();
        direct += rec.material->eval(wi, dir_l, rec.normal, rec.u, rec.v) * cos_theta * Le_l / light_pdf_sa;
      }
    }
  }

  // Environment map — one importance-sampled direction, no MIS.
  if (scene.env_map() && scene.env_map()->valid()) {
    Vec3  env_dir;
    float env_pdf;
    Color Le_env = scene.env_map()->sample_light(rng, env_dir, env_pdf);
    if (env_pdf > 0.0f) {
      Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, env_dir);
      if (!scene.occluded(shadow_ray, RAY_EPSILON, RAY_TMAX)) {
        float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, env_dir));
        direct += rec.material->eval(wi, env_dir, rec.normal, rec.u, rec.v) * cos_theta * Le_env / env_pdf;
      }
    }
  }

  return direct;
}

}  // namespace kestrel
