/**
 * @file image_io.cpp
 * @brief PPM / PNG / EXR framebuffer writers. This is the single translation
 *        unit that defines the stb_image_write and tinyexr implementations.
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 0
#include <zlib.h>
#include "tinyexr.h"

#include "io/image_io.h"
#include "logger.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>

namespace kestrel {

namespace {

float gamma_encode(float v) {
  return std::pow(std::min(std::max(v, 0.0f), 1.0f), 1.0f / 2.2f);
}

void write_ppm(const std::string &filename, const std::vector<Color> &pixels,
               int width, int height) {
  std::ofstream file(filename);
  file << "P3\n" << width << " " << height << "\n255\n";
  for (int j = height - 1; j >= 0; --j) {
    for (int i = 0; i < width; ++i) {
      const Color &p = pixels[j * width + i];
      file << static_cast<int>(255.99f * gamma_encode(p.x)) << " "
           << static_cast<int>(255.99f * gamma_encode(p.y)) << " "
           << static_cast<int>(255.99f * gamma_encode(p.z)) << "\n";
    }
  }
}

void write_png(const std::string &filename, const std::vector<Color> &pixels,
               int width, int height) {
  std::vector<uint8_t> buf(width * height * 3);
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      const Color &p = pixels[j * width + i];
      int idx = (j * width + i) * 3;
      buf[idx + 0] = static_cast<uint8_t>(255.99f * gamma_encode(p.x));
      buf[idx + 1] = static_cast<uint8_t>(255.99f * gamma_encode(p.y));
      buf[idx + 2] = static_cast<uint8_t>(255.99f * gamma_encode(p.z));
    }
  }
  // Pixel buffer stores row 0 at the bottom; PNG is top-to-bottom.
  // Negative stride tells stb to walk rows in reverse.
  stbi_write_png(filename.c_str(), width, height, 3,
                 buf.data() + static_cast<ptrdiff_t>((height - 1) * width * 3),
                 -width * 3);
}

void write_exr(const std::string &filename, const std::vector<Color> &pixels,
               int width, int height) {
  // EXR stores linear float data — no gamma, clamp only negatives.
  // Build an interleaved RGB float array, flipping rows to top-to-bottom.
  std::vector<float> buf(width * height * 3);
  for (int j = 0; j < height; ++j) {
    int exr_row = height - 1 - j;
    for (int i = 0; i < width; ++i) {
      const Color &p = pixels[j * width + i];
      int idx = (exr_row * width + i) * 3;
      buf[idx + 0] = std::max(p.x, 0.0f);
      buf[idx + 1] = std::max(p.y, 0.0f);
      buf[idx + 2] = std::max(p.z, 0.0f);
    }
  }
  const char *err = nullptr;
  int ret = SaveEXR(buf.data(), width, height, 3, /*fp16=*/0,
                    filename.c_str(), &err);
  if (ret != TINYEXR_SUCCESS) {
    LOG_WARNING("EXR write failed: " + std::string(err ? err : "unknown error"));
    FreeEXRErrorMessage(err);
  }
}

} // namespace

void write_image(const std::string &path, const std::vector<Color> &pixels,
                 int width, int height) {
  auto dot = path.rfind('.');
  std::string ext = (dot != std::string::npos) ? path.substr(dot) : "";
  if      (ext == ".png") write_png(path, pixels, width, height);
  else if (ext == ".exr") write_exr(path, pixels, width, height);
  else                    write_ppm(path, pixels, width, height);
}

}  // namespace kestrel
