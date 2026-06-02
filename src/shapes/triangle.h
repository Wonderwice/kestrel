/**
 * @class Triangle
 * @brief A class representing a triangle.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "shape.h"
#include "math/mat4.h"

namespace kestrel {

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
           const Vec3 &n0, const Vec3 &n1, const Vec3 &n2,
           const Material *material,
           float tu0 = 0.f, float tv0 = 0.f,
           float tu1 = 1.f, float tv1 = 0.f,
           float tu2 = 0.f, float tv2 = 1.f)
      : Shape(material), v0(v0), v1(v1), v2(v2), n0(n0), n1(n1), n2(n2),
        tu0(tu0), tv0(tv0), tu1(tu1), tv1(tv1), tu2(tu2), tv2(tv2) {
    compute_bounds();
  }

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

  /// Any-hit test: Möller–Trumbore returning on the first valid t in range,
  /// skipping the normal/UV/tangent computation that hit() performs.
  bool occluded(const Ray &ray, float t_min, float t_max) const override;

  /// Uniformly sample a point on the triangle and return the solid-angle PDF
  /// for the direction from `from` toward it (area lights / NEE).
  bool sample_light(const Point3 &from, PCG32 &rng,
                    Vec3 &sample_point, float &pdf_sa) const override;

  /// Solid-angle PDF of sampling direction `dir` from `from` toward this
  /// triangle (0 if `dir` misses it). Used for MIS against BSDF sampling.
  float light_pdf(const Point3 &from, const Vec3 &dir) const override;

  /**
   * @brief Scale the triangle by a factor along each axis
   * @param factor Scaling factor for each axis
   */
  virtual void scale(const Vec3 &factor) override;

  /**
   * @brief Translate the triangle by an offset vector
   * @param offset Translation vector
   */
  virtual void translate(const Vec3 &offset) override;


  /**
   * @brief Rotate the triangle around a specified axis by a given angle.
   * @param angle_degrees Rotation angle in degrees.
   * @param axis Axis of rotation (should be a normalized vector).
   */
  virtual void rotate(float angle_degrees, const Vec3 &axis) override;

  /**
   * @brief Apply an affine transform to the vertex positions and shading
   * normals (normals via the inverse-transpose of the upper-left 3x3).
   */
  void transform(const Mat4 &m);

  private:
  Point3 v0, v1, v2;           ///< Vertices of the triangle
  Vec3 n0, n1, n2;             ///< Normal vectors of the triangle vertices
  float tu0, tv0;              ///< UV of vertex 0
  float tu1, tv1;              ///< UV of vertex 1
  float tu2, tv2;              ///< UV of vertex 2
  Vec3 e1_, e2_;               ///< Cached edges v1-v0, v2-v0 (Möller–Trumbore);
                               ///< recomputed by compute_bounds() on any edit.

    /**
   * @brief Generate the boundaries of the shape.
   */
  virtual void compute_bounds() override;
};

}  // namespace kestrel
