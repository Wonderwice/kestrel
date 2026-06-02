#include "camera.h"
#include "constants.h"
#include "ray.h"
#include <cmath>

namespace kestrel {

Ray Camera::get_ray(float u, float v, PCG32 &rng) const {
  Point3 focus_pt = lower_left_corner + u * horizontal + v * vertical;

  Vec3 offset(0, 0, 0);
  if (lens_radius > 0.0f) {
    float r     = std::sqrt(rng.next_float()) * lens_radius;
    float theta = TWO_PI * rng.next_float();
    offset = u_axis * (r * std::cos(theta)) + v_axis * (r * std::sin(theta));
  }

  Point3 new_origin = origin + offset;
  return Ray(new_origin, focus_pt - new_origin);
}

Camera::Camera(Point3 look_from, Point3 look_at, Vec3 vup, float vfov,
               int width, float aspect_ratio,
               float aperture, float focal_dist) {
  float h = std::tan(vfov * DEG_TO_RAD / 2.0f);
  float viewport_height = 2.0f * h;
  float viewport_width  = aspect_ratio * viewport_height;

  Vec3 w = (look_from - look_at).normalized();
  u_axis = Vec3::cross(vup, w).normalized();
  v_axis = Vec3::cross(w, u_axis);

  // Scale the focal plane by focal_dist so it lies at that distance.
  // FOV is preserved: the angular span is atan(h) regardless of scale.
  horizontal       = viewport_width  * focal_dist * u_axis;
  vertical         = viewport_height * focal_dist * v_axis;
  lower_left_corner = look_from - horizontal / 2.0f - vertical / 2.0f - w * focal_dist;

  origin      = look_from;
  lens_radius = aperture / 2.0f;
  _width      = width;
  _height     = static_cast<int>(width / aspect_ratio);
}

}  // namespace kestrel
