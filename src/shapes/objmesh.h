/**
 * @class ObjMesh
 * @brief Reprensentation of a .obj file
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "material.h"
#include "shape.h"
#include "mesh.h"
#include "triangle.h"
#include <vector>

namespace kestrel {

class ObjMesh : public Mesh{
public:
  /**
   * @brief ObjMesh constructor
   * @param filepath Path of the .obj file
   * @param material Material applied to the shape
   */
  ObjMesh(std::string filepath, const Material *material);

};


}  // namespace kestrel
