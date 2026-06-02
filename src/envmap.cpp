#include "envmap.h"
#include "constants.h"
#include "logger.h"
#include "stb_image.h"
#include "tinyexr.h"
#include <algorithm>
#include <cmath>

// ─── loading ────────────────────────────────────────────────────────────────

namespace kestrel {

EnvMap::EnvMap(const std::string &filepath, float scale) : scale_(scale) {
  auto dot = filepath.rfind('.');
  std::string ext = (dot != std::string::npos) ? filepath.substr(dot) : "";

  if (ext == ".exr") {
    float *data = nullptr;
    const char *err = nullptr;
    int ret = LoadEXR(&data, &width, &height, filepath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
      LOG_WARNING("EnvMap: failed to load EXR " + filepath +
                  (err ? (": " + std::string(err)) : ""));
      FreeEXRErrorMessage(err);
      return;
    }
    pixels.resize(width * height * 3);
    for (int i = 0; i < width * height; ++i) {
      pixels[i*3+0] = data[i*4+0];
      pixels[i*3+1] = data[i*4+1];
      pixels[i*3+2] = data[i*4+2];
    }
    free(data);
  } else {
    stbi_set_flip_vertically_on_load(0);
    if (stbi_is_hdr(filepath.c_str())) {
      int ch;
      float *data = stbi_loadf(filepath.c_str(), &width, &height, &ch, 3);
      if (!data) { LOG_WARNING("EnvMap: failed to load " + filepath); return; }
      pixels.assign(data, data + width * height * 3);
      stbi_image_free(data);
    } else {
      int ch;
      uint8_t *data = stbi_load(filepath.c_str(), &width, &height, &ch, 3);
      if (!data) { LOG_WARNING("EnvMap: failed to load " + filepath); return; }
      pixels.resize(width * height * 3);
      for (int i = 0; i < width * height * 3; ++i)
        pixels[i] = std::pow(data[i] / 255.0f, 2.2f);
      stbi_image_free(data);
    }
  }
  build_cdfs();
}

// ─── CDF construction ────────────────────────────────────────────────────────

void EnvMap::build_cdfs() {
  marginal_cdf.resize(height + 1, 0.0f);
  conditional_cdf.resize(height * (width + 1), 0.0f);

  total_importance = 0.0f;
  for (int j = 0; j < height; ++j) {
    float theta     = PI * (j + 0.5f) / height;
    float sin_theta = std::sin(theta);

    float *cdf_row = conditional_cdf.data() + j * (width + 1);
    cdf_row[0] = 0.0f;
    for (int i = 0; i < width; ++i) {
      int k = (j * width + i) * 3;
      float l = lum(pixels[k], pixels[k+1], pixels[k+2]) * sin_theta;
      cdf_row[i+1] = cdf_row[i] + l;
    }
    float row_sum = cdf_row[width];
    if (row_sum > 0.0f)
      for (int i = 1; i <= width; ++i)
        cdf_row[i] /= row_sum;

    total_importance += row_sum;
    marginal_cdf[j+1] = total_importance;
  }
  if (total_importance > 0.0f)
    for (int j = 1; j <= height; ++j)
      marginal_cdf[j] /= total_importance;
}

// ─── direction ↔ UV ──────────────────────────────────────────────────────────

// Mitsuba's equirectangular convention (matches mitsuba 0.6 and mitsuba 3):
//   u = atan2(x, -z) / 2π,   v = acos(y) / π
// with +Y the pole/zenith and -Z mapping to u = 0. atan2 returns [-π,π] so u may
// be negative ([-0.5, 0.5]); callers wrap it into [0,1) (lookup() does, and pdf()
// wraps before indexing). uv_to_dir() below is the exact inverse.
static void dir_to_uv(const Vec3 &d, float &u, float &v) {
  u = std::atan2(d.x, -d.z) * (0.5f / PI);
  v = std::acos(std::max(-1.0f, std::min(1.0f, d.y))) / PI;
}

// ─── bilinear lookup ─────────────────────────────────────────────────────────

Color EnvMap::lookup(float u, float v) const {
  u = u - std::floor(u); // wrap U
  v = std::max(0.0f, std::min(1.0f, v));

  float fx = u * width  - 0.5f;
  float fy = v * height - 0.5f;
  int x0 = static_cast<int>(std::floor(fx));
  int y0 = static_cast<int>(std::floor(fy));
  float tx = fx - x0, ty = fy - y0;
  int x1 = (x0 + 1) % width;
  x0 = ((x0 % width) + width) % width;
  int y1 = std::min(y0 + 1, height - 1);
  y0 = std::max(y0, 0);

  auto px = [&](int x, int y) {
    int i = (y * width + x) * 3;
    return Color(pixels[i], pixels[i+1], pixels[i+2]);
  };
  return px(x0,y0)*(1-tx)*(1-ty) + px(x1,y0)*tx*(1-ty)
       + px(x0,y1)*(1-tx)*ty     + px(x1,y1)*tx*ty;
}

// ─── public API ──────────────────────────────────────────────────────────────

Color EnvMap::sample(const Vec3 &dir) const {
  if (pixels.empty()) return Color(0, 0, 0);
  float u, v;
  dir_to_uv(to_local(dir), u, v);
  return lookup(u, v) * scale_;
}

Color EnvMap::sample_light(PCG32 &rng, Vec3 &out_dir, float &out_pdf) const {
  // Invert marginal CDF → row j
  float r1 = rng.next_float();
  int j = static_cast<int>(
    std::upper_bound(marginal_cdf.begin(), marginal_cdf.end(), r1) - marginal_cdf.begin()
  ) - 1;
  j = std::max(0, std::min(j, height - 1));

  // Invert conditional CDF for row j → column i
  const float *cdf_row = conditional_cdf.data() + j * (width + 1);
  float r2 = rng.next_float();
  int i = static_cast<int>(
    std::upper_bound(cdf_row, cdf_row + width + 1, r2) - cdf_row
  ) - 1;
  i = std::max(0, std::min(i, width - 1));

  // Pixel centre → direction (inverse of dir_to_uv; Mitsuba convention).
  float phi       = TWO_PI * (i + 0.5f) / width;
  float theta     = PI    * (j + 0.5f) / height;
  float sin_theta = std::sin(theta);
  out_dir = Vec3(sin_theta * std::sin(phi), std::cos(theta), -sin_theta * std::cos(phi));
  out_dir = to_world(out_dir);  // env-local -> world

  if (sin_theta < 1e-6f) { out_pdf = 0.0f; return Color(0,0,0); }

  // p(ω) = lum(i,j) * W*H / (total_importance * 2π²)
  int k = (j * width + i) * 3;
  float l = lum(pixels[k], pixels[k+1], pixels[k+2]);
  out_pdf = l * (width * height) / (total_importance * TWO_PI * PI);

  return Color(pixels[k], pixels[k+1], pixels[k+2]) * scale_;
}

float EnvMap::pdf(const Vec3 &dir_world) const {
  Vec3 dir = to_local(dir_world);
  if (total_importance <= 0.0f) return 0.0f;
  float u, v;
  dir_to_uv(dir, u, v);
  u = u - std::floor(u);  // wrap [-0.5,0.5] -> [0,1) to match the sampling indexing
  int i = std::max(0, std::min(static_cast<int>(u * width),  width  - 1));
  int j = std::max(0, std::min(static_cast<int>(v * height), height - 1));
  float theta = PI * (j + 0.5f) / height;
  float sin_theta = std::sin(theta);
  if (sin_theta < 1e-6f) return 0.0f;
  int k = (j * width + i) * 3;
  float l = lum(pixels[k], pixels[k+1], pixels[k+2]);
  return l * (width * height) / (total_importance * TWO_PI * PI);
}

}  // namespace kestrel
