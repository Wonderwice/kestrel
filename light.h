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
  Light(const Vec3 &position, const Vec3 &intensity)
      : position(position), intensity(intensity) {}

  Vec3 sample_direction(const Vec3 &point) const {
    // Sample direction from point to light position
    return (position - point).normalized();
  }


  Vec3 get_intensity() const { return intensity; }

  Vec3 position;  ///< Position of the light source
  Vec3 intensity; ///< Color/intensity of the light source
};

#endif