/**
 * @file image_io.h
 * @brief Write a linear-RGB framebuffer to disk (PPM / PNG / EXR).
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "vec3.h"
#include <string>
#include <vector>

/**
 * @brief Write `pixels` (linear RGB, row 0 at the bottom) to `path`.
 *
 * The output format is chosen by the file extension:
 *  - `.exr` : 32-bit linear float (no gamma), HDR-capable
 *  - `.png` : 8-bit sRGB (gamma 2.2)
 *  - anything else : ASCII PPM (gamma 2.2)
 */
namespace kestrel {

void write_image(const std::string &path, const std::vector<Color> &pixels,
                 int width, int height);

}  // namespace kestrel
