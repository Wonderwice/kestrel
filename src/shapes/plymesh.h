/**
 * @class PlyMesh
 * @brief Reprensentation of a .ply file
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "material.h"
#include "shape.h"
#include "triangle.h"
#include "mesh.h"
#include <vector>

namespace kestrel {

class PlyMesh : public Mesh {
public:
  /**
   * @brief Plymesh constructor
   * @param filepath Path of the .ply file
   * @param material Material applied to the shape
   */
  PlyMesh(std::string filepath, const Material *material);
  ~PlyMesh() {}
};


}  // namespace kestrel
