#include "lambertian.h"
#include "constants.h"

namespace kestrel {

bool Lambertian::scatter(const Ray &incoming, const HitRecord &rec,
                         Color &attenuation, Ray &scattered,
                         float &pdf_out, PCG32 &rng) const {
  Vec3 shading_normal = rec.normal;

  if (normal_map) {
    // Decode tangent-space normal from texture [0,1]^3 → [-1,1]^3
    Color nm = normal_map->value(rec.u, rec.v);
    Vec3 ts_n(nm.x * 2.0f - 1.0f, nm.y * 2.0f - 1.0f, nm.z * 2.0f - 1.0f);

    // Build TBN matrix from hit-point geometry
    Vec3 T = rec.tangent.normalized();
    Vec3 N = rec.normal;
    T = (T - N * Vec3::dot(T, N)).normalized(); // Gram-Schmidt orthogonalise
    Vec3 B = Vec3::cross(N, T);

    // Transform tangent-space normal to world space
    shading_normal = (T * ts_n.x + B * ts_n.y + N * ts_n.z).normalized();
    if (!rec.front_face) shading_normal = shading_normal * -1.0f;
  }

  Vec3 scatter_direction = shading_normal + Vec3::random_unit_vector(rng);
  scattered = Ray(rec.point, scatter_direction);
  // f(wi,wo)*cos / pdf = (albedo/π)*cos / (cos/π) = albedo
  attenuation = texture->value(rec.u, rec.v);
  // Cosine-weighted hemisphere pdf about the shading normal.
  pdf_out = std::max(0.0f, Vec3::dot(scatter_direction.normalized(), shading_normal)) * INV_PI;
  return true;
}

Color Lambertian::get_color() const { return texture->value(0, 0) * INV_PI; }

float Lambertian::pdf(const Vec3 &wi, const Vec3 &wo, const Vec3 &n) const {
  float cos_theta = std::max(0.0f, Vec3::dot(wo.normalized(), n));
  return cos_theta * INV_PI;
}

Color Lambertian::eval(const Vec3 & /*wi*/, const Vec3 & /*wo*/, const Vec3 & /*n*/,
                       float u, float v) const {
  return texture->value(u, v) * INV_PI;
}

}  // namespace kestrel
