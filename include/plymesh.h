/**
 * @class PlyMesh
 * @brief Reprensentation of a .ply file
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef PLYMESH_H
#define PLYMESH_H

#include "bsdfs/material.h"
#include "shape.h"
#include "triangle.h"
#include <vector>

class PlyMesh : public Shape {
public:

  /**
   * @brief Plymesh constructor
   * @param filepath Path of the .ply file
   * @param material Material applied to the shape
   */
  PlyMesh(std::string filepath, const Material *material);

  /**
   * @brief Pure virtual method to test ray-shape intersection
   * @param ray The ray to test for intersection
   * @param t_min Minimum valid t parameter (near clipping)
   * @param t_max Maximum valid t parameter (far clipping)
   * @param rec Output parameter filled with intersection details if hit occurs
   * @return True if intersection exists in range [t_min, t_max], false
   * otherwise
   */
  virtual bool hit(const Ray &ray, float t_min, float t_max,
                   HitRecord &rec) const;

  /**
   * @brief Scale the entire mesh by a factor along each axis
   * @param factor Scaling factor for each axis
   */
  void scale(Vec3 factor);

  /**
   * @brief Translate the entire mesh by an offset vector
   * @param offset Translation vector
   */
  void translate(Vec3 offset);

  private:
  // Add member variables to store vertices and faces as needed
  std::vector<Triangle> triangles;
};

#endif