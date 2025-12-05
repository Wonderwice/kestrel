/**
 * @class Triangle
 * @brief A class representing a triangle.
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "shape.h"

class Triangle : public Shape {
public:
  /**
   * @brief Construct a new Triangle object
   * @param v0 First vertex of the triangle
   * @param v1 Second vertex of the triangle
   * @param v2 Third vertex of the triangle
   * @param material Material of the triangle
   */
  Triangle(const Point3 &v0, const Point3 &v1, const Point3 &v2,
           const Material *material)
      : v0(v0), v1(v1), v2(v2), Shape(material) {}

  /**
   * @brief Test ray-triangle intersection using Möller–Trumbore algorithm
   * @param ray The ray to test for intersection
   * @param t_min Minimum valid t parameter (near clipping)
   * @param t_max Maximum valid t parameter (far clipping)
   * @param rec Output parameter filled with intersection details if hit occurs
   * @return True if intersection exists in range [t_min, t_max], false
   * otherwise
   */
  bool hit(const Ray &ray, float t_min, float t_max,
           HitRecord &rec) const override;

  /**
   * @brief Scale the triangle by a factor along each axis
   * @param factor Scaling factor for each axis
   */
  void scale(Vec3 factor);

  /**
   * @brief Translate the triangle by an offset vector
   * @param offset Translation vector
   */
  void translate(Vec3 offset);

  private:
  Point3 v0, v1, v2; ///< Vertices of the triangle
};

#endif // TRIANGLE_H