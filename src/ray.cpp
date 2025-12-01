#include "ray.h"

HOST_DEVICE Ray::Ray(const Point3 &origin, const Vec3 &direction)
    : origin(origin), direction(direction) {}

HOST_DEVICE Point3 Ray::at(float t) const { return origin + t * direction; }