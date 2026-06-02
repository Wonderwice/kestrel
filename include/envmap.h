#pragma once

#include "pcg32.h"
#include "vec3.h"
#include <string>
#include <vector>

/**
 * Equirectangular HDR environment map with 2D importance sampling.
 * Supports Radiance .hdr (stb_image) and .exr (tinyexr).
 */
namespace kestrel {

class EnvMap {
public:
  // `scale` multiplies the emitted radiance (Mitsuba's envmap "scale" param).
  explicit EnvMap(const std::string &filepath, float scale = 1.0f);
  bool valid() const { return !pixels.empty(); }

  // Set the local->world rotation (Mitsuba's envmap to_world). c0/c1/c2 are the
  // world-space images of the env-local x/y/z axes (columns of the rotation).
  void set_rotation(const Vec3 &c0, const Vec3 &c1, const Vec3 &c2) {
    c0_ = c0; c1_ = c1; c2_ = c2;
  }

  // Evaluate radiance in direction dir (must be normalised).
  Color sample(const Vec3 &dir) const;

  // Importance-sample a direction; sets out_dir and solid-angle PDF.
  Color sample_light(PCG32 &rng, Vec3 &out_dir, float &out_pdf) const;

  // Solid-angle PDF for a given normalised direction.
  float pdf(const Vec3 &dir) const;

private:
  std::vector<float> pixels;          // RGB linear floats, row-major top-down
  int width = 0, height = 0;
  float scale_ = 1.0f;                // radiance multiplier
  // local->world rotation columns (identity by default).
  Vec3 c0_{1, 0, 0}, c1_{0, 1, 0}, c2_{0, 0, 1};

  // env-local <- world (R^T) and world <- env-local (R).
  Vec3 to_local(const Vec3 &d) const {
    return Vec3(Vec3::dot(d, c0_), Vec3::dot(d, c1_), Vec3::dot(d, c2_));
  }
  Vec3 to_world(const Vec3 &d) const { return c0_ * d.x + c1_ * d.y + c2_ * d.z; }

  std::vector<float> marginal_cdf;    // H+1 elements
  std::vector<float> conditional_cdf; // H * (W+1), row-major
  float total_importance = 0.0f;      // sum of lum*sin(theta) over all pixels

  void build_cdfs();
  Color lookup(float u, float v) const;

  static float lum(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  }
};

}  // namespace kestrel
