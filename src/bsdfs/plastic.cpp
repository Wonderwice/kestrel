#include "plastic.h"
#include "constants.h"
#include "pcg32.h"

namespace kestrel {

// Internal multiple-scattering compensation for the coated-diffuse base. invEta2
// is the radiance-scaling factor for light crossing the dielectric boundary; the
// 1/(1 - Fdr) term sums the geometric series of diffusely-scattered light that is
// internally reflected at the boundary and re-scatters off the base before
// escaping. Mitsuba's `plastic` exposes a "nonlinear" switch for this series:
//   nonlinear=false (DEFAULT): scalar  rho * invEta2 / (1 - fdr_int)
//   nonlinear=true           : per-channel  rho_c * invEta2 / (1 - rho_c*fdr_int)
// The default (linear) preserves the base texture's hue; the nonlinear form lets
// saturated bases darken/shift as they would under repeated colored bounces.
static Color coated_diffuse_gain(const Color &rho, float fdr_int, float inv_eta2,
                                 bool nonlinear) {
  if (nonlinear) {
    auto ch = [&](float c) { return c * inv_eta2 / (1.0f - c * fdr_int); };
    return Color(ch(rho.x), ch(rho.y), ch(rho.z));
  }
  return rho * (inv_eta2 / (1.0f - fdr_int));
}

bool Plastic::scatter(const Ray &incoming, const HitRecord &rec,
                      Color &attenuation, Ray &scattered,
                      float &pdf_out, PCG32 &rng) const {
  Vec3 wi = (incoming.direction * -1.0f).normalized();
  float cos_theta = std::max(0.0f, Vec3::dot(rec.normal, wi));
  float F = coat_fresnel(cos_theta);

  if (rng.next_float() < F) {
    // Specular coat: perfect mirror reflection (delta lobe). Picking it with
    // probability F and weighting by 1 reproduces the Fresnel reflectance F.
    Vec3 in_dir = incoming.direction.normalized();
    Vec3 wo = in_dir - 2.0f * Vec3::dot(in_dir, rec.normal) * rec.normal;
    scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
    attenuation = Color(1, 1, 1);
    pdf_out     = 0.0f; // delta: no NEE/MIS
    return true;
  }

  // Diffuse base under the dielectric coat. Light transmits through the coat
  // on the way IN (the (1−F_i) selection probability, which cancels the entry
  // transmittance) and on the way OUT, so the exit transmittance (1−F(cosθ_o))
  // for the sampled direction remains in the weight.
  Vec3 wo = (rec.normal + Vec3::random_unit_vector(rng)).normalized();
  float fo = coat_fresnel(std::max(0.0f, Vec3::dot(wo, rec.normal)));
  scattered   = Ray(rec.point + rec.normal * RAY_EPSILON, wo);
  // Throughput weight f*cos/pdf: the diffuse selection probability (1-F) cancels
  // the entry transmittance (1-fi) and the cosine cancels the cosine pdf, leaving
  // the internal-scattering gain times the exit transmittance (1-fo).
  attenuation = coated_diffuse_gain(texture->value(rec.u, rec.v), fdr_int, inv_eta2,
                                    nonlinear_)
              * (1.0f - fo);
  pdf_out = (1.0f - F) * std::max(0.0f, Vec3::dot(wo, rec.normal)) * INV_PI;
  return true;
}

Color Plastic::eval(const Vec3 &wi, const Vec3 &wo, const Vec3 &n,
                    float u, float v) const {
  // Coated diffuse: attenuated by the coat's Fresnel transmittance entering
  // (view side) AND exiting (light side). The coat itself is a delta (not seen
  // by NEE).
  float fi = coat_fresnel(std::max(0.0f, Vec3::dot(n, wi)));
  float fo = coat_fresnel(std::max(0.0f, Vec3::dot(n, wo)));
  Color rho = texture->value(u, v);
  return coated_diffuse_gain(rho, fdr_int, inv_eta2, nonlinear_)
       * (INV_PI * (1.0f - fi) * (1.0f - fo));
}

float Plastic::pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const {
  float F = coat_fresnel(std::max(0.0f, Vec3::dot(n, wi)));
  return (1.0f - F) * std::max(0.0f, Vec3::dot(wo, n)) * INV_PI;
}

}  // namespace kestrel
