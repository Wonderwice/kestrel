/**
 * @file scene.h
 * @brief Scene representation with objects and camera
 * @author Alexei Czornyj
 * @date 2025
 */

#ifndef SCENE_H
#define SCENE_H

#include "camera.h"
#include "light.h"
#include "shape.h"
#include <vector>

/**
 * @brief Represents a 3D scene with a camera and objects
 */
class Scene {
public:
  Camera camera;               ///< Camera used for rendering
  std::vector<const Shape*> objects; ///< List of objects in the scene
  std::vector<Light> lights;   ///< List of lights in the scene

  /**
   * @brief Construct a new Scene object
   * @param cam Camera to use for the scene
   */
  HOST_DEVICE Scene(const Camera &cam) : camera(cam) {}

  /**
   * @brief Add an object to the scene
   * @param obj Shape to add
   */
  HOST_DEVICE void add_object(const Shape *obj) { objects.push_back(obj); }

   /**
   * @brief Add a light to the scene
   * @param light Light to add
   */
  HOST_DEVICE void add_light(const Light &light) { lights.push_back(light); }

  /**
   * @brief Test ray against all objects in the scene for intersection
   * @param ray The ray to test
   * @param t_min Minimum valid t parameter
   * @param t_max Maximum valid t parameter
   * @param rec Output parameter filled with closest intersection details
   * @return True if any intersection occurs, false otherwise
   *
   * Iterates over all objects in the scene and finds the closest intersection
   * point along the ray within the specified t range.
   */
  HOST_DEVICE bool hit(const Ray &ray, float t_min, float t_max,
                       HitRecord &rec) const;
};

#endif // SCENE_H
