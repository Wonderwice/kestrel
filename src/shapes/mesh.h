
#pragma once

#include "triangle.h"
#include "shape.h"
#include "accel/bvh.h"
#include "math/mat4.h"
#include <vector>

namespace kestrel {

class Mesh : public Shape{
    public:
        Mesh(const Material * mat) : Shape(mat) {};
        virtual ~Mesh() = 0;

        const std::vector<Triangle> &get_triangles() const { return triangles; }

        virtual bool hit(const Ray &ray, float t_min, float t_max,
                HitRecord &rec) const;

        // Any-hit test for shadow rays: stops at the first occluding triangle.
        virtual bool occluded(const Ray &ray, float t_min, float t_max) const;

        void scale(const Vec3 &factor);
        void translate(const Vec3 &offset);
        void rotate(float angle_degrees, const Vec3 &axis);

        // Apply an affine transform to every triangle (vertices + normals).
        void transform(const Mat4 &m);

    protected:
        std::vector<Triangle> triangles;

        virtual void compute_bounds();

    private:
        void build_bvh();
        BVH bvh_; ///< Bounding-volume hierarchy over `triangles`.
};

}  // namespace kestrel
