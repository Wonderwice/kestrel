#include "path_integrator.h"
#include "constants.h"
#include "scene.h"

// Deterministic multiple importance sampling (Veach's multi-sample model):
// every non-specular bounce takes ONE sample from each of two strategies and
// sums their power-heuristic-weighted contributions — it never randomly picks a
// single strategy. The two strategies are:
//   1. Light sampling (NEE): a shadow ray to a sampled point on an area emitter
//      / environment map, weighted by w = power(p_light, p_bsdf).
//   2. BSDF sampling: the scattered continuation ray; when it lands on an
//      emitter (handled at the top of the next loop iteration) its emission is
//      weighted by w = power(p_bsdf, p_light).
// Specular/delta lobes (pdf == 0) skip strategy 1 and carry w = 1 in strategy 2.
// For single-emitter scenes the two strategies' pdfs are mutually exact; with
// several area lights NEE weights with the selected light's pdf while the BSDF
// strategy uses the full light-mixture pdf (scene.emissive_pdf) — both unbiased.
namespace kestrel {

Color PathIntegrator::integrate(const Ray &initial_ray, const Scene &scene, PCG32 &rng) const {
  Color radiance(0, 0, 0);
  Color throughput(1, 1, 1);
  Ray ray = initial_ray;

  float prev_bsdf_pdf = 1.0f;
  bool  last_delta    = true;

  for (int depth = 0; depth < max_depth; ++depth) {
    HitRecord rec;
    if (!scene.hit(ray, RAY_EPSILON, RAY_TMAX, rec)) {
      if (scene.env_map() && scene.env_map()->valid()) {
        Color Le = scene.env_map()->sample(ray.direction.normalized());
        float w = 1.0f;
        if (!last_delta) {
          float env_pdf = scene.env_map()->pdf(ray.direction.normalized());
          w = mis_power(prev_bsdf_pdf, env_pdf);
        }
        radiance += throughput * Le * w;
      } else {
        radiance += throughput * scene.background_color();
      }
      break;
    }

    // Emissive surface hit via BSDF sampling — MIS-weight against NEE.
    // Area lights emit from their front face only (HW3 §4); a ray landing on
    // the back face sees a black, non-emitting surface and the path ends.
    Color Le = rec.material->emitted();
    if (Le.x > 0 || Le.y > 0 || Le.z > 0) {
      if (rec.front_face) {
        float w = 1.0f;
        if (!last_delta) {
          float light_pdf = scene.emissive_pdf(ray.origin, ray.direction.normalized());
          w = mis_power(prev_bsdf_pdf, light_pdf);
        }
        radiance += throughput * Le * w;
      }
      break;
    }

    if (!rec.material->is_specular()) {
      Vec3 wi = (ray.direction * -1.0f).normalized();

      // Point lights (delta distributions — MIS weight = 1).
      for (const auto &light : scene.lights()) {
        Vec3  to_light  = light.position - rec.point;
        float dist      = to_light.length();
        Vec3  light_dir = to_light / dist;

        Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, light_dir);
        if (scene.occluded(shadow_ray, RAY_EPSILON, dist - RAY_EPSILON))
          continue;

        float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, light_dir));
        float dist_sq   = dist * dist + DISTANCE_EPSILON;
        radiance += throughput * rec.material->eval(wi, light_dir, rec.normal, rec.u, rec.v)
                    * cos_theta * light.get_intensity() / dist_sq;
      }

      // Area emitters — MIS with BSDF sampling.
      if (!scene.emissives().empty()) {
        int  n   = static_cast<int>(scene.emissives().size());
        int  idx = std::min(static_cast<int>(rng.next_float() * n), n - 1);
        const Shape *light_shape = scene.emissives()[idx];

        Vec3  sample_pt;
        float light_pdf_sa;
        if (light_shape->sample_light(rec.point, rng, sample_pt, light_pdf_sa)) {
          light_pdf_sa /= n;
          Vec3  to_sample = sample_pt - rec.point;
          float dist_l    = to_sample.length();
          Vec3  dir_l     = to_sample / dist_l;

          Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, dir_l);
          bool blocked = scene.occluded(shadow_ray, RAY_EPSILON, dist_l - RAY_EPSILON);

          if (!blocked && light_pdf_sa > 0.0f) {
            float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, dir_l));
            Color brdf      = rec.material->eval(wi, dir_l, rec.normal, rec.u, rec.v);
            float bsdf_pdf  = rec.material->pdf(wi, dir_l, rec.normal);
            float w         = mis_power(light_pdf_sa, bsdf_pdf);
            Color Le_l      = light_shape->material->emitted();
            radiance += throughput * brdf * cos_theta * Le_l * w / light_pdf_sa;
          }
        }
      }

      // Environment map — MIS with BSDF sampling.
      if (scene.env_map() && scene.env_map()->valid()) {
        Vec3  env_dir;
        float env_pdf;
        Color Le_env = scene.env_map()->sample_light(rng, env_dir, env_pdf);
        if (env_pdf > 0.0f) {
          Ray shadow_ray(rec.point + rec.geo_normal * SHADOW_BIAS, env_dir);
          bool blocked = scene.occluded(shadow_ray, RAY_EPSILON, RAY_TMAX);
          if (!blocked) {
            float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, env_dir));
            Color brdf      = rec.material->eval(wi, env_dir, rec.normal, rec.u, rec.v);
            float bsdf_pdf  = rec.material->pdf(wi, env_dir, rec.normal);
            float w         = mis_power(env_pdf, bsdf_pdf);
            radiance += throughput * brdf * cos_theta * Le_env * w / env_pdf;
          }
        }
      }
    }

    // Russian roulette after 3 bounces.
    if (depth >= 3) {
      float q = std::min(std::max({throughput.x, throughput.y, throughput.z}), 0.95f);
      if (rng.next_float() > q) break;
      throughput /= q;
    }

    // BSDF scatter.
    Color attenuation;
    Ray   scattered;
    float sampled_pdf = 0.0f;
    if (!rec.material->scatter(ray, rec, attenuation, scattered, sampled_pdf, rng)) break;

    prev_bsdf_pdf = sampled_pdf;
    last_delta    = (sampled_pdf <= 0.0f);

    throughput *= attenuation;
    ray = scattered;
  }
  return radiance;
}

}  // namespace kestrel
