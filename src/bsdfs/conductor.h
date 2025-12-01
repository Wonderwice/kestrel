/**
 * @file conductor.h
 * @brief Conductor reflection model for ray tracing
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include "kestrel.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"

/**
 * @class Conductor
 * @brief Implements conductor reflection model
 * This class provides methods to compute reflection directions
 * for conductive materials like metals. It generates perfect
 * reflection directions based on the incoming ray and surface normal.
 */
class Conductor : public Material {
public:
  /**
   * @brief Default constructor
   */
  HOST_DEVICE Conductor() {}

  /**
   * @brief Construct a Conductor with specified albedo color
   * @param albedo Reflective color of the conductor surface
   */
  HOST_DEVICE Conductor(Color albedo) : albedo(albedo), Material(1.0f) {}

  /**
   *  * @brief Generate a reflection direction
   * @param incoming Incoming ray direction
   * @param rec Hit record with surface normal
   * @param attenuation Output color attenuation
   * @param scattered Output scattered ray
   * @return True if scattering occurred
   *
   * Computes the perfect reflection direction based on the incoming ray
   * and the surface normal at the hit point.
   */
  HOST_DEVICE bool scatter(const Ray &incoming, const HitRecord &rec,
                           Color &attenuation, Ray &scattered) const {
    Vec3 scatter_direction =
        incoming.direction -
        2.0f * Vec3::dot(incoming.direction, rec.normal) * rec.normal;
    scattered = Ray(rec.point, scatter_direction);
    attenuation = albedo;
    return true;
  }

  /**
   * @brief Get the color of the conductor
   * @return Albedo color
   */
  HOST_DEVICE Color get_color() const { return albedo; }

private:
  Color albedo;
};

#endif // CONDUCTOR_H
