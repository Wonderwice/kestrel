#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tinyexr.h"
#include "texture.h"
#include "logger.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace kestrel {

ImageTexture::ImageTexture(const std::string &filepath) {
  auto dot = filepath.rfind('.');
  std::string ext = (dot == std::string::npos) ? "" : filepath.substr(dot);
  for (char &ch : ext) ch = std::tolower(static_cast<unsigned char>(ch));

  if (ext == ".exr") {
    float *data = nullptr;
    const char *err = nullptr;
    int ret = LoadEXR(&data, &width, &height, filepath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
      LOG_WARNING("ImageTexture EXR load failed: " + filepath
                  + (err ? std::string(" — ") + err : ""));
      if (err) FreeEXRErrorMessage(err);
      return;
    }
    // LoadEXR returns RGBA float in linear space.
    pixels.resize(width * height * 3);
    for (int i = 0; i < width * height; ++i) {
      pixels[3 * i + 0] = data[4 * i + 0];
      pixels[3 * i + 1] = data[4 * i + 1];
      pixels[3 * i + 2] = data[4 * i + 2];
    }
    free(data);
    return;
  }

  int channels;
  uint8_t *data = stbi_load(filepath.c_str(), &width, &height, &channels, 3);
  if (!data) {
    LOG_WARNING("ImageTexture: failed to load " + filepath);
    return;
  }
  pixels.resize(width * height * 3);
  for (int i = 0; i < width * height * 3; ++i)
    pixels[i] = std::pow(data[i] / 255.0f, 2.2f); // sRGB -> linear
  stbi_image_free(data);
}

Color ImageTexture::value(float u, float v) const {
  if (pixels.empty())
    return Color(1, 0, 1); // magenta sentinel for missing texture

  // Apply UV scale/offset (Mitsuba uscale/vscale/uoffset/voffset), then wrap.
  u = uscale * u + uoffset;
  v = vscale * v + voffset;
  u = u - std::floor(u);
  v = v - std::floor(v);
  // Flip V: image row 0 is top, but v=0 is bottom in the renderer
  v = 1.0f - v;

  // Guard against non-finite UVs (e.g. from a degenerate ray direction): a NaN
  // would survive the wrap above and yield an out-of-bounds pixel index.
  if (!std::isfinite(u) || !std::isfinite(v))
    return Color(0, 0, 0);

  float fx = u * (width  - 1);
  float fy = v * (height - 1);
  int x0 = std::min(std::max(static_cast<int>(fx), 0), width  - 1);
  int y0 = std::min(std::max(static_cast<int>(fy), 0), height - 1);
  int x1 = std::min(x0 + 1, width  - 1);
  int y1 = std::min(y0 + 1, height - 1);
  float tx = fx - x0, ty = fy - y0;

  auto px = [&](int x, int y) -> Color {
    int i = (y * width + x) * 3;
    return Color(pixels[i], pixels[i+1], pixels[i+2]);
  };

  // Bilinear interpolation
  return px(x0,y0)*(1-tx)*(1-ty) + px(x1,y0)*tx*(1-ty)
       + px(x0,y1)*(1-tx)*ty     + px(x1,y1)*tx*ty;
}

}  // namespace kestrel
