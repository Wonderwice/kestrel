#pragma once

#include "vec3.h"
#include <cmath>
#include <string>
#include <vector>

namespace kestrel {

class Texture {
public:
  virtual ~Texture() = default;
  virtual Color value(float u, float v) const = 0;
};

class ConstantTexture : public Texture {
public:
  ConstantTexture(Color c) : albedo(c) {}
  Color value(float, float) const override { return albedo; }
private:
  Color albedo;
};

/**
 * @class CheckerboardTexture
 * @brief Two-colour checker pattern in UV space (Mitsuba's "checkerboard").
 *
 * The (u,v) coordinates are scaled by (uscale,vscale) before the checker test,
 * matching Mitsuba's <transform name="to_uv"><scale .../></transform>.
 */
class CheckerboardTexture : public Texture {
public:
  CheckerboardTexture(Color color0, Color color1,
                      float uscale = 1.f, float vscale = 1.f)
      : color0(color0), color1(color1), uscale(uscale), vscale(vscale) {}

  Color value(float u, float v) const override {
    int iu = static_cast<int>(std::floor(u * uscale));
    int iv = static_cast<int>(std::floor(v * vscale));
    return ((iu + iv) & 1) == 0 ? color0 : color1;
  }

private:
  Color color0, color1;
  float uscale, vscale;
};

class ImageTexture : public Texture {
public:
  ImageTexture(const std::string &filepath);
  bool valid() const { return !pixels.empty(); }
  Color value(float u, float v) const override;

  // Scale/offset applied to UVs before wrapping, matching Mitsuba's
  // uscale/vscale/uoffset/voffset: u' = uscale*u + uoffset, then modulo 1.
  void set_uv_transform(float us, float vs, float uo, float vo) {
    uscale = us; vscale = vs; uoffset = uo; voffset = vo;
  }
private:
  std::vector<float> pixels; // linearised RGB floats, row-major top-down
  int width = 0, height = 0;
  float uscale = 1.f, vscale = 1.f, uoffset = 0.f, voffset = 0.f;
};

}  // namespace kestrel
