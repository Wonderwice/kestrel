/**
 * @file material.h
 * @brief Material interface for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include "kestrel.h"
#include "ray.h"
#include "vec3.h"

class HitRecord;
/**
 * @class Material
 * @brief Material base class for ray tracing
 */
class Material {
public:
  /**
   * @brief Default constructor
   * @param reflectivity Reflection for mirror materials
   */
  HOST_DEVICE Material(float reflectivity = 0.0f)
      : reflectivity(reflectivity) {}

  /**
   * @brief Scatter an incoming ray
   * @param incoming Incoming ray direction
   * @param rec Hit record with surface normal
   * @param attenuation Output color attenuation
   * @param scattered Output scattered ray
   * @return True if scattering occurred
   */
  HOST_DEVICE virtual bool scatter(const Ray &incoming, const HitRecord &rec,
                                   Color &attenuation,
                                   Ray &scattered) const = 0;

  /**
   * @brief Get the color of the material
   * @return Material color
   */
  HOST_DEVICE virtual Color get_color() const = 0;

  float reflectivity; ///< Reflection for mirror materials
};

#endif