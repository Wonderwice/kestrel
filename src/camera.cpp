#include "camera.h"
#include "ray.h"

Ray Camera::get_ray(float u, float v) const {
  Vec3 ray_direction =
      lower_left_corner + u * horizontal + v * vertical - origin;
  return Ray(origin, ray_direction);
}

Camera::Camera(Point3 look_from, Point3 look_at, Vec3 vup, float vfov,
               float aspect_ratio)  {
    // Calculate viewport dimensions
    float theta = vfov * M_PI / 180.0f;
    float h = std::tan(theta / 2.0f);
    float viewport_height = 2.0f * h;
    float viewport_width = aspect_ratio * viewport_height;

    // Calculate camera basis vectors (right-handed coordinate system)
    Vec3 w = (look_from - look_at).normalized(); // Points away from look_at
    Vec3 u = Vec3::cross(vup, w).normalized();   // Points right
    Vec3 v = Vec3::cross(w, u);                  // Points up

    origin = look_from;
    horizontal = viewport_width * u;
    vertical = viewport_height * v;
    lower_left_corner = origin - horizontal / 2.0f - vertical / 2.0f - w;
  }