#include "shape.h"
#include "accel/aabb.h"
#include "math/mat4.h"

/**
 * @file rectangle.h
 * @brief Rectangle primitive header.
 */

/**
 * @class Rectangle
 * @brief A quadrilateral shape represented by four corner vertices.
 */
namespace kestrel {

class Rectangle : public Shape
{
private:
    /// Upper-left corner position.
    Vec3 upper_left;
    /// Upper-right corner position.
    Vec3 upper_right;
    /// Bottom-left corner position.
    Vec3 bottom_left;
    /// Bottom-right corner position.
    Vec3 bottom_right;

    /**
     * @brief Compute the axis-aligned bounding box and centroid.
     *
     * Expands the internal `_bounds` to include all four corners and
     * sets `_centroid` to the center of the bounding box.
     */
    void compute_bounds() override {
        _bounds = AABB();
        _bounds.expand(upper_left);
        _bounds.expand(upper_right);
        _bounds.expand(bottom_left);
        _bounds.expand(bottom_right);
        _centroid = (_bounds.min + _bounds.max) * 0.5f;
    }

public:
    /**
     * @brief Construct a Rectangle with the given material.
     * @param material Pointer to the material applied to the rectangle.
     */
    Rectangle(const Material *material);
    
    /**
     * @brief Ray-primitive intersection test.
     * @param ray The ray to test against.
     * @param t_min Minimum t value to consider.
     * @param t_max Maximum t value to consider.
     * @param rec Hit record to populate if an intersection occurs.
     * @return true if the ray hits the rectangle, false otherwise.
     */
    bool hit(const Ray &ray, float t_min, float t_max,
             HitRecord &rec) const override;

    /// Uniformly sample a point on the rectangle for area-light NEE, returning
    /// the solid-angle PDF for the direction from `from`. Two-sided.
    bool sample_light(const Point3 &from, PCG32 &rng,
                      Vec3 &sample_point, float &pdf_sa) const override;

    /// Solid-angle PDF of sampling direction `dir` from `from` toward this
    /// rectangle (0 if `dir` misses it). Used for MIS against BSDF sampling.
    float light_pdf(const Point3 &from, const Vec3 &dir) const override;

    ~Rectangle() override = default;

    /**
     * @brief Scale the rectangle by a per-axis factor about the origin.
     * @param factor Per-axis scale factors.
     */
    virtual void scale(const Vec3 &factor) override;

    /**
     * @brief Translate the rectangle by the given offset.
     * @param offset Translation vector to add to all corners.
     */
    virtual void translate(const Vec3 &offset) override;

    virtual void rotate(float rotate_angle, const Vec3 &rotate_axis) override;

    /**
     * @brief Apply an affine transform to the four corner positions.
     */
    void transform(const Mat4 &m) {
        upper_left   = m.transform_point(upper_left);
        upper_right  = m.transform_point(upper_right);
        bottom_left  = m.transform_point(bottom_left);
        bottom_right = m.transform_point(bottom_right);
        compute_bounds();
    }
};

}  // namespace kestrel
