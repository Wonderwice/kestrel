/**
 * @class Shape
 * @brief Abstract base class for geometric shapes. Defines the interface for
 * ray-shape intersection tests.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "material.h"
#include "ray.h"
#include "vec3.h"
#include "accel/aabb.h"

namespace kestrel {

class Shape
{
protected:
  AABB _bounds; ///< Material of the shape
  Vec3 _centroid; ///< Centroid used for the BVH

  /**
   * @brief Generate the boundaries of the shape.
   */
  virtual void compute_bounds() = 0;

public:
  /**
   * @brief Constructor of the abstract Shape class
   * @param material Material applied on the shape
   */
  Shape(const Material *material) : material(material) {}
  /**
   * @brief Virtual destructor
   */
  virtual ~Shape() = default;
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
                   HitRecord &rec) const = 0;

  /**
   * @brief Any-hit occlusion test for shadow / NEE rays.
   *
   * Returns true as soon as *any* intersection exists in [t_min, t_max]; it does
   * not search for the closest hit and fills no HitRecord. The default discards a
   * throwaway record from hit(); Triangle/Mesh override it to also skip the
   * shading-normal / UV / tangent work that hit() does.
   */
  virtual bool occluded(const Ray &ray, float t_min, float t_max) const {
    HitRecord tmp;
    return hit(ray, t_min, t_max, tmp);
  }

  virtual void scale(const Vec3 &factor) = 0;
  virtual void translate(const Vec3 &offset) = 0;
  virtual void rotate(float rotate_angle, const Vec3 &rotate_axis) = 0;

  const AABB bounds() const {return _bounds;}

  const Vec3 centroid() const {return _centroid;}

  /**
   * @brief Sample a direction toward this shape for light sampling (MIS).
   * @param from Origin point from which we are sampling
   * @param rng  Random number generator
   * @param sample_point Output: the sampled point on the shape surface
   * @param pdf_sa Output: solid-angle PDF for the sampled direction
   * @return true if a valid sample was produced
   */
  virtual bool sample_light(const Point3 &from, PCG32 &rng,
                             Vec3 &sample_point, float &pdf_sa) const {
    return false;
  }

  /**
   * @brief Evaluate the solid-angle PDF for a given direction toward this shape.
   * @param from  Origin point
   * @param dir   Normalized direction toward the shape
   * @return Solid-angle PDF (0 if direction misses)
   */
  virtual float light_pdf(const Point3 &from, const Vec3 &dir) const {
    return 0.0f;
  }

  const Material *material; ///< Material of the shape
};

}  // namespace kestrel
