/**
 * @file light.h
 * @brief Light source representation
 * @author Alexei Czornyj
 * @date 2025
 *
 * This file defines the Light class, which represents a light source
 * in the scene. It provides methods to sample light direction and intensity.
 */

#ifndef LIGHT_H
#define LIGHT_H

#include "vec3.h"

class Light {
public:
  /**
   * @brief Construct a light source
   * @param position Position of the light in 3D space
   * @param intensity Color/intensity of the light
   */
  Light(const Vec3 &position, const Vec3 &intensity)
      : position(position), intensity(intensity) {}

  /**
   * @brief Sample direction from a point to the light source
   * @param point Point in space from which to sample the light direction
   * @return Normalized direction vector from point to light
   */
  Vec3 sample_direction(const Vec3 &point) const;

  Vec3 get_intensity() const;

  Vec3 position;  ///< Position of the light source
  Vec3 intensity; ///< Color/intensity of the light source
};

#endif