/**
 * @file camera.h
 * @brief Pinhole camera model for ray generation
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef CAMERA_H
#define CAMERA_H

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
class Camera {
private:
  Point3 origin;            ///< Camera position in world space
  Point3 lower_left_corner; ///< Bottom-left corner of image plane
  Vec3 horizontal;          ///< Horizontal span of image plane
  Vec3 vertical;            ///< Vertical span of image plane
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
   *
   * The camera uses a right-handed coordinate system. The look_at point
   * defines the center of the view, and vup defines which direction is "up"
   * in the image.
   */
  Camera(Point3 look_from, Point3 look_at, Vec3 vup, float vfov,
         int width, float aspect_ratio);

  /**
   * @brief Generate a ray for a given pixel coordinate
   * @param u Horizontal coordinate in [0, 1] (0 = left, 1 = right)
   * @param v Vertical coordinate in [0, 1] (0 = bottom, 1 = top)
   * @return Ray from camera origin through the (u, v) position on image plane
   *
   * The parameters (u, v) represent normalized coordinates on the image plane.
   * (0, 0) is the bottom-left corner, (1, 1) is the top-right corner.
   */
  Ray get_ray(float u, float v) const;

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

#endif