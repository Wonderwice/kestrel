/**
 * @file camera.h
 * @brief Pinhole camera model for ray generation
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include "ray.h"
#include "vec3.h"

/**
 * @class Camera
 * @brief Perspective camera with configurable field of view
 *
 * Implements a standard pinhole camera model with configurable position,
 * orientation, and field of view. Uses a right-handed coordinate system
 * where the camera looks down the negative Z axis.
 */
namespace kestrel {

class Camera {
private:
  Point3 origin;            ///< Camera position in world space
  Point3 lower_left_corner; ///< Bottom-left corner of focal plane
  Vec3 horizontal;          ///< Horizontal span of focal plane
  Vec3 vertical;            ///< Vertical span of focal plane
  Vec3 u_axis;              ///< Camera right vector (for lens disk sampling)
  Vec3 v_axis;              ///< Camera up vector (for lens disk sampling)
  float lens_radius;        ///< Aperture radius (0 = pinhole)
  int _width;              ///< Image width in pixels
  int _height;             ///< Image height in pixels

public:
  /**
   * @brief Construct a perspective camera
   * @param look_from Camera position in world space
   * @param look_at Point the camera is looking at
   * @param vup "Up" direction vector (typically (0, 1, 0))
   * @param vfov Vertical field of view in degrees
   * @param width Image width in pixels
   * @param aspect_ratio Image aspect ratio (width / height)
   * @param aperture Lens diameter; 0 gives a pinhole camera
   * @param focal_dist Distance to the plane of perfect focus
   */
  Camera(Point3 look_from, Point3 look_at, Vec3 vup, float vfov,
         int width, float aspect_ratio,
         float aperture = 0.0f, float focal_dist = 1.0f);

  /**
   * @brief Generate a ray for a given pixel coordinate
   * @param u Horizontal coordinate in [0, 1]
   * @param v Vertical coordinate in [0, 1]
   * @param rng Random number generator (used for lens disk sampling)
   * @return Ray from a point on the lens toward the focal plane
   */
  Ray get_ray(float u, float v, PCG32 &rng) const;

  /**
   * @brief Get image width in pixels
   * @return Image width
   */
  int width() const { return _width; }
  /**
   * @brief Get image height in pixels
   * @return Image height
   */
  int height() const { return _height; }
};

}  // namespace kestrel
