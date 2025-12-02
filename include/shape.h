/**
 * @class Shape
 * @brief Abstract base class for geometric shapes. Defines the interface for
 * ray-shape intersection tests.
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef SHAPE_H
#define SHAPE_H

#include "bsdfs/material.h"
#include "ray.h"
#include "vec3.h"

class Shape {
public:
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
  const Material *material; ///< Material of the shape
};

#endif // SHAPE_H