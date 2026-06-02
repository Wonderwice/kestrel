#pragma once

namespace kestrel {

constexpr float PI                   = 3.14159265358979323846f;
constexpr float INV_PI               = 1.0f / PI;
constexpr float TWO_PI               = 2.0f * PI;
constexpr float DEG_TO_RAD           = PI / 180.0f;

constexpr float RAY_EPSILON          = 0.001f;
// Distance a shadow / NEE ray origin is pushed off the surface along the
// geometric normal. Larger than RAY_EPSILON so it clears neighbouring facets on
// coarse meshes (avoids self-shadow acne), but small relative to scene features
// so it does not leak light through thin geometry.
constexpr float SHADOW_BIAS          = 0.01f;
constexpr float INTERSECTION_EPSILON = 1e-8f;
constexpr float DISTANCE_EPSILON     = 1e-4f;

// Default far clip / maximum ray travel distance.
constexpr float RAY_TMAX             = 5000.0f;

}  // namespace kestrel
