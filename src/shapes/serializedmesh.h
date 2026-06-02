/**
 * @class SerializedMesh
 * @brief Loads a single mesh from a Mitsuba .serialized file by shape index.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "material.h"
#include "mesh.h"
#include <string>

namespace kestrel {

class SerializedMesh : public Mesh {
public:
  /**
   * @param filepath    Path to the .serialized file.
   * @param shape_index Which mesh inside the file to load.
   * @param material    Material applied to all triangles of this shape.
   */
  SerializedMesh(std::string filepath, int shape_index, const Material *material);
};

}  // namespace kestrel
