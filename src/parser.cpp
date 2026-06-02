#include "blinn.h"
#include "blinn_microfacet.h"
#include "conductor.h"
#include "dielectric.h"
#include "emissive.h"
#include "envmap.h"
#include "lambertian.h"
#include "microfacet.h"
#include "phong.h"
#include "plastic.h"
#include "rough_dielectric.h"
#include "texture.h"
#include "kestrel.h"
#include "logger.h"
#include "mesh.h"
#include "plymesh.h"
#include "objmesh.h"
#include "rectangle.h"
#include "scene.h"
#include "serializedmesh.h"
#include "sphere.h"

#include "tinyxml2.h"

#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "constants.h"
#include "math/mat4.h"
#include "scene_params.h"

namespace kestrel {

using namespace tinyxml2;

namespace {

// ── Stateless XML attribute helpers ──────────────────────────────────────────

Vec3 parse_rgb(const XMLElement *elem) {
    const char *val = elem ? elem->Attribute("value") : nullptr;
    if (!val) return Vec3(0, 0, 0);
    // Accept "R, G, B" or "R G B"
    std::string s(val);
    for (char &c : s) if (c == ',') c = ' ';
    float r = 0, g = 0, b = 0;
    int n = sscanf(s.c_str(), "%f %f %f", &r, &g, &b);
    if (n == 1) g = b = r;   // a single value is greyscale (Mitsuba convention)
    return Vec3(r, g, b);
}

float get_float(const XMLElement *elem, float def = 0.f) {
    return elem ? elem->FloatAttribute("value", def) : def;
}

int get_int(const XMLElement *elem, int def = 0) {
    return elem ? elem->IntAttribute("value", def) : def;
}

std::string get_str(const XMLElement *elem) {
    if (!elem) return "";
    const char *v = elem->Attribute("value");
    return v ? v : "";
}

// Parse x= y= z= attributes; missing attributes keep the default component.
Vec3 parse_xyz(const XMLElement *elem, Vec3 def = Vec3(0, 0, 0)) {
    if (!elem) return def;
    Vec3 out = def;
    out.x = elem->FloatAttribute("x", def.x);
    out.y = elem->FloatAttribute("y", def.y);
    out.z = elem->FloatAttribute("z", def.z);
    return out;
}

// Find the first child element whose name= attribute equals `name`.
const XMLElement *find_named(const XMLElement *parent, const char *tag, const char *name) {
    for (auto *e = parent->FirstChildElement(tag); e; e = e->NextSiblingElement(tag))
        if (e->Attribute("name", name)) return e;
    return nullptr;
}

// Compose <transform> children into a single matrix (and/or extract lookAt).
void parse_transform(const XMLElement *xform,
                     bool &has_transform,
                     Vec3 *lookfrom = nullptr,
                     Vec3 *lookat_out = nullptr,
                     Vec3 *vup = nullptr,
                     bool *has_matrix = nullptr,
                     Mat4 *matrix = nullptr) {
    if (!xform) return;

    // Compose all transform ops into a single matrix in *document order*: the
    // first-listed op is applied first to the point (Mitsuba semantics), so the
    // composed matrix is acc = op_n * ... * op_1, built by left-multiplying.
    Mat4 acc = Mat4::identity();
    bool any_op = false;

    for (auto *c = xform->FirstChildElement(); c; c = c->NextSiblingElement()) {
        const char *tag = c->Name();

        // Case-insensitive lookat (scenes use both "lookAt" and "lookat")
        if (strcasecmp(tag, "lookat") == 0) {
            if (!lookfrom) continue;
            auto parse_csv3 = [](const char *s, Vec3 &out) {
                if (!s) return;
                std::string str(s);
                for (char &ch : str) if (ch == ',') ch = ' ';
                sscanf(str.c_str(), "%f %f %f", &out.x, &out.y, &out.z);
            };
            parse_csv3(c->Attribute("origin"), *lookfrom);
            parse_csv3(c->Attribute("target"), *lookat_out);
            parse_csv3(c->Attribute("up"),     *vup);
            continue;
        }

        Mat4 op;
        if (strcmp(tag, "scale") == 0) {
            op = Mat4::scaling(parse_xyz(c, Vec3(1, 1, 1)));
            has_transform = true;
        } else if (strcmp(tag, "translate") == 0) {
            op = Mat4::translation(parse_xyz(c, Vec3(0, 0, 0)));
            has_transform = true;
        } else if (strcmp(tag, "rotate") == 0) {
            op = Mat4::rotation(c->FloatAttribute("angle", 0.f),
                                parse_xyz(c, Vec3(0, 0, 0)));
            has_transform = true;
        } else if (strcmp(tag, "matrix") == 0) {
            const char *val = c->Attribute("value");
            if (!val) continue;
            // Mitsuba matrices are 16 whitespace-separated floats, row-major.
            std::string s(val);
            for (char &ch : s) if (ch == ',') ch = ' ';
            std::istringstream iss(s);
            int n = 0;
            float v;
            float vals[16];
            while (n < 16 && (iss >> v)) vals[n++] = v;
            if (n != 16) { LOG_ERROR("matrix value did not contain 16 floats"); continue; }
            op = Mat4::from_row_major(vals);
        } else {
            continue; // Unknown tag silently skipped.
        }

        acc = op * acc;
        any_op = true;
    }

    if (any_op && matrix && has_matrix) {
        *matrix = acc;
        *has_matrix = true;
    }
}

// ── Mitsuba scene loader ──────────────────────────────────────────────────────
//
// Holds the parse-time state (base directory, id→BSDF and id→texture maps, and
// the scene under construction) so the per-element parsers don't thread it all
// through their signatures. Geometry and BSDFs are handed to the Scene, which
// takes ownership; shared textures are co-owned via shared_ptr.
class MitsubaLoader {
public:
    std::unique_ptr<Scene> load(const std::string &filepath);

private:
    std::string base_dir_;
    Scene *scene_ = nullptr;
    std::map<std::string, Material *> bsdf_map_;
    std::map<std::string, std::shared_ptr<const Texture>> texture_map_;
    std::map<std::string, std::string> defaults_;  // Mitsuba <default> params

    // Recursively replace $name / ${name} in all attributes using defaults_.
    void substitute_defaults(XMLElement *elem);

    std::shared_ptr<ImageTexture> make_image_texture(const XMLElement *tex_elem) const;
    std::shared_ptr<const Texture> make_checkerboard(const XMLElement *tex_elem) const;
    // Dispatch on the element's type= attribute (bitmap / checkerboard).
    std::shared_ptr<const Texture> make_texture(const XMLElement *tex_elem) const;
    std::shared_ptr<const Texture> parse_reflectance(const XMLElement *bsdf_elem, Color fallback);
    Material *parse_bsdf_element(const XMLElement *bsdf_elem);
    Material *resolve_shape_material(const XMLElement *shape_elem);
    void parse_sphere(const XMLElement *shape_elem);
    void parse_rectangle(const XMLElement *shape_elem);
    void parse_mesh(const XMLElement *shape_elem, bool is_ply);
    void parse_serialized(const XMLElement *shape_elem);
    void parse_cube(const XMLElement *shape_elem);
    void parse_sensor(const XMLElement *elem);
};

// Build an ImageTexture from a <texture type="bitmap"> element, applying its
// uscale/vscale/uoffset/voffset UV transform. Returns nullptr if no filename.
std::shared_ptr<ImageTexture> MitsubaLoader::make_image_texture(const XMLElement *tex_elem) const {
    const XMLElement *fn = find_named(tex_elem, "string", "filename");
    if (!fn) return nullptr;
    auto img = std::make_shared<ImageTexture>(base_dir_ + get_str(fn));
    img->set_uv_transform(get_float(find_named(tex_elem, "float", "uscale"),  1.f),
                          get_float(find_named(tex_elem, "float", "vscale"),  1.f),
                          get_float(find_named(tex_elem, "float", "uoffset"), 0.f),
                          get_float(find_named(tex_elem, "float", "voffset"), 0.f));
    return img;
}

// Build a CheckerboardTexture from a <texture type="checkerboard"> element.
// color0/color1 default to Mitsuba's 0.4 / 0.2; the optional
// <transform name="to_uv"><scale x= y=/></transform> sets the checker scale.
std::shared_ptr<const Texture> MitsubaLoader::make_checkerboard(const XMLElement *tex_elem) const {
    Color c0(0.4f, 0.4f, 0.4f), c1(0.2f, 0.2f, 0.2f);
    if (const XMLElement *e = find_named(tex_elem, "rgb", "color0")) c0 = parse_rgb(e);
    if (const XMLElement *e = find_named(tex_elem, "rgb", "color1")) c1 = parse_rgb(e);

    float us = 1.f, vs = 1.f;
    if (const XMLElement *xf = find_named(tex_elem, "transform", "to_uv")) {
        if (const XMLElement *sc = xf->FirstChildElement("scale")) {
            us = sc->FloatAttribute("x", 1.f);
            vs = sc->FloatAttribute("y", 1.f);
        }
    }
    // Mitsuba's checkerboard fits two cells (one color0 + one color1) per unit of
    // the to_uv scale, whereas CheckerboardTexture uses one cell per unit
    // (floor(u*scale)). Double the scale on import so a Mitsuba scene's checker
    // frequency is reproduced (verified against the matpreview reference).
    return std::make_shared<CheckerboardTexture>(c0, c1, us * 2.f, vs * 2.f);
}

// Dispatch a <texture> element on its type= attribute.
std::shared_ptr<const Texture> MitsubaLoader::make_texture(const XMLElement *tex_elem) const {
    const char *type = tex_elem->Attribute("type");
    if (type && strcmp(type, "checkerboard") == 0) return make_checkerboard(tex_elem);
    return make_image_texture(tex_elem); // default: bitmap
}

// Extract a "reflectance" parameter as a shared texture, supporting <rgb>,
// inline <texture> bitmaps, and <ref id> to a shared texture.
std::shared_ptr<const Texture> MitsubaLoader::parse_reflectance(const XMLElement *bsdf_elem,
                                                                Color fallback) {
    for (auto *c = bsdf_elem->FirstChildElement(); c; c = c->NextSiblingElement()) {
        const char *cname = c->Attribute("name");
        // "diffuse_reflectance" is Mitsuba's name for the plastic base albedo.
        if (!cname || (strcmp(cname, "reflectance") != 0 &&
                       strcmp(cname, "diffuse_reflectance") != 0)) continue;
        const char *ctag = c->Name();
        if (strcmp(ctag, "rgb") == 0) {
            return std::make_shared<ConstantTexture>(parse_rgb(c));
        } else if (strcmp(ctag, "texture") == 0) {
            auto img = make_texture(c);
            if (img) return img;
        } else if (strcmp(ctag, "ref") == 0) {
            const char *rid = c->Attribute("id");
            if (rid && texture_map_.count(rid)) return texture_map_[rid];
            if (rid) LOG_ERROR("texture reference '" + std::string(rid) + "' not found");
        }
    }
    return std::make_shared<ConstantTexture>(fallback);
}

Material *MitsubaLoader::parse_bsdf_element(const XMLElement *bsdf_elem) {
    const char *type_attr = bsdf_elem->Attribute("type");
    if (!type_attr) { LOG_ERROR("BSDF element missing type attribute"); return nullptr; }
    std::string type(type_attr);

    // Unwrap twosided — kestrel has no two-sided material
    if (type == "twosided") {
        const XMLElement *inner = bsdf_elem->FirstChildElement("bsdf");
        if (inner) return parse_bsdf_element(inner);
        LOG_ERROR("twosided bsdf has no inner bsdf");
        return nullptr;
    }

    Material *mat = nullptr;

    if (type == "diffuse") {
        float r = 0, g = 0, b = 0;
        std::string bitmap_path, normalmap_path;
        std::shared_ptr<const Texture> ref_texture = nullptr;
        const XMLElement *refl_tex_elem = nullptr;

        for (auto *c = bsdf_elem->FirstChildElement(); c; c = c->NextSiblingElement()) {
            const char *cname = c->Attribute("name");
            if (!cname) continue;

            if (strcmp(cname, "reflectance") == 0) {
                const char *ctag = c->Name();
                if (strcmp(ctag, "rgb") == 0) {
                    Vec3 col = parse_rgb(c);
                    r = col.x; g = col.y; b = col.z;
                } else if (strcmp(ctag, "texture") == 0) {
                    refl_tex_elem = c; // built below, with its uv transform
                } else if (strcmp(ctag, "ref") == 0) {
                    const char *rid = c->Attribute("id");
                    if (rid && texture_map_.count(rid)) {
                        ref_texture = texture_map_[rid];
                    } else if (rid) {
                        LOG_ERROR("texture reference '" + std::string(rid) + "' not found");
                    }
                }
            } else if (strcmp(cname, "normalmap") == 0 || strcmp(cname, "normal_map") == 0) {
                const XMLElement *fn_elem = find_named(c, "string", "filename");
                if (fn_elem) normalmap_path = get_str(fn_elem);
            }
        }

        // Fallback: bare <string name="filename"> at top level
        if (bitmap_path.empty() && normalmap_path.empty() && !ref_texture && !refl_tex_elem) {
            const XMLElement *fn_elem = find_named(bsdf_elem, "string", "filename");
            if (fn_elem) bitmap_path = get_str(fn_elem);
        }

        Lambertian *lam;
        if (ref_texture)
            lam = new Lambertian(ref_texture);
        else if (refl_tex_elem)
            lam = new Lambertian(make_texture(refl_tex_elem));
        else if (!bitmap_path.empty())
            lam = new Lambertian(std::make_shared<ImageTexture>(base_dir_ + bitmap_path));
        else
            lam = new Lambertian(Color(r, g, b));

        if (!normalmap_path.empty())
            lam->set_normal_map(std::make_shared<ImageTexture>(base_dir_ + normalmap_path));

        mat = lam;

    } else if (type == "mirror" || type == "conductor") {
        Vec3 col(1, 1, 1);
        const XMLElement *refl = find_named(bsdf_elem, "rgb", "reflectance");
        if (refl) col = parse_rgb(refl);
        mat = new Conductor(col);

    } else if (type == "dielectric" || type == "glass") {
        float ior = 1.5f;
        const XMLElement *ior_elem = find_named(bsdf_elem, "float", "intIOR");
        if (!ior_elem) ior_elem = find_named(bsdf_elem, "float", "ior");
        if (ior_elem) ior = get_float(ior_elem, 1.5f);
        mat = new Dielectric(ior);

    } else if (type == "roughdielectric" || type == "roughglass") {
        float ior = 1.5f;
        const XMLElement *ior_elem = find_named(bsdf_elem, "float", "intIOR");
        if (!ior_elem) ior_elem = find_named(bsdf_elem, "float", "ior");
        if (ior_elem) ior = get_float(ior_elem, 1.5f);
        float roughness = 0.1f;
        const XMLElement *alpha = find_named(bsdf_elem, "float", "alpha");
        if (alpha) roughness = get_float(alpha, 0.1f);
        Color tint(1, 1, 1);
        const XMLElement *refl = find_named(bsdf_elem, "rgb", "specularReflectance");
        if (refl) tint = parse_rgb(refl);
        mat = new RoughDielectric(tint, roughness, ior);

    } else if (type == "roughconductor" || type == "roughplastic") {
        Vec3 col(0.8f, 0.8f, 0.8f);
        float roughness = 0.3f;
        const XMLElement *refl = find_named(bsdf_elem, "rgb", "reflectance");
        if (!refl) refl = find_named(bsdf_elem, "rgb", "eta");
        if (refl) col = parse_rgb(refl);
        const XMLElement *alpha = find_named(bsdf_elem, "float", "alpha");
        if (alpha) roughness = get_float(alpha, 0.3f);
        mat = new Microfacet(col, roughness);

    } else if (type == "phong" || type == "blinn" || type == "blinn_microfacet") {
        auto tex = parse_reflectance(bsdf_elem, Color(0.8f, 0.8f, 0.8f));
        float exponent = 50.0f;
        const XMLElement *exp = find_named(bsdf_elem, "float", "exponent");
        if (exp) exponent = get_float(exp, 50.0f);
        if (type == "phong")       mat = new Phong(tex, exponent);
        else if (type == "blinn")  mat = new Blinn(tex, exponent);
        else                       mat = new BlinnMicrofacet(tex, exponent);

    } else if (type == "plastic") {
        auto tex = parse_reflectance(bsdf_elem, Color(0.5f, 0.5f, 0.5f));
        float eta = 1.5f;
        const XMLElement *eta_elem = find_named(bsdf_elem, "float", "eta");
        if (!eta_elem) eta_elem = find_named(bsdf_elem, "float", "intIOR");
        if (!eta_elem) eta_elem = find_named(bsdf_elem, "float", "int_ior");
        if (eta_elem) eta = get_float(eta_elem, 1.5f);
        // Mitsuba's "nonlinear" switch (default false): colored internal scattering.
        bool nonlinear = false;
        const XMLElement *nl = find_named(bsdf_elem, "boolean", "nonlinear");
        if (nl) { const char *v = nl->Attribute("value"); nonlinear = v && strcmp(v, "true") == 0; }
        mat = new Plastic(tex, eta, nonlinear);

    } else if (type == "area" || type == "emitter") {
        Vec3 col(1, 1, 1);
        const XMLElement *rad = find_named(bsdf_elem, "rgb", "radiance");
        if (!rad) rad = find_named(bsdf_elem, "rgb", "emission");
        if (rad) col = parse_rgb(rad);
        mat = new Emissive(col);

    } else {
        LOG_ERROR("Unsupported BSDF type: " + type);
        return nullptr;
    }

    if (mat) scene_->add_bsdf(mat);
    return mat;
}

// Resolve the material for a shape: inline <emitter type="area"> wins, then
// <ref>, then an inline <bsdf>.
Material *MitsubaLoader::resolve_shape_material(const XMLElement *shape_elem) {
    // Inline area emitter takes priority: a shape carrying <emitter type="area">
    // is a light source, even when it also declares a <ref>/<bsdf> (Mitsuba
    // scenes attach both). Our Emissive material is non-scattering, so emission
    // wins over the reflectance material.
    const XMLElement *em = shape_elem->FirstChildElement("emitter");
    if (em && em->Attribute("type", "area")) {
        Vec3 col(1, 1, 1);
        const XMLElement *rad = find_named(em, "rgb", "radiance");
        if (rad) col = parse_rgb(rad);
        Material *mat = new Emissive(col);
        scene_->add_bsdf(mat);
        return mat;
    }

    // <ref id="..."/>
    const XMLElement *ref = shape_elem->FirstChildElement("ref");
    if (ref) {
        const char *id = ref->Attribute("id");
        if (id && bsdf_map_.count(id)) return bsdf_map_[id];
        if (id) LOG_ERROR("BSDF reference '" + std::string(id) + "' not found");
    }

    // Inline <bsdf>
    const XMLElement *bsdf = shape_elem->FirstChildElement("bsdf");
    if (bsdf) return parse_bsdf_element(bsdf);

    return nullptr;
}

// ── Shape parsers ────────────────────────────────────────────────────────────

void MitsubaLoader::parse_sphere(const XMLElement *shape_elem) {
    Vec3 center(0, 0, 0);
    float radius = 1.f;

    const XMLElement *c_elem = find_named(shape_elem, "point", "center");
    if (c_elem) center = parse_xyz(c_elem);

    const XMLElement *r_elem = find_named(shape_elem, "float", "radius");
    if (r_elem) radius = get_float(r_elem, 1.f);

    Material *mat = resolve_shape_material(shape_elem);
    scene_->add_object(new Sphere(center, radius, mat));
}

void MitsubaLoader::parse_rectangle(const XMLElement *shape_elem) {
    bool has_transform = false;
    bool has_matrix = false;
    Mat4 matrix;

    const XMLElement *xform = shape_elem->FirstChildElement("transform");
    parse_transform(xform, has_transform, nullptr, nullptr, nullptr, &has_matrix, &matrix);

    Material *mat = resolve_shape_material(shape_elem);

    Rectangle *rect = new Rectangle(mat);
    // parse_transform composes the ops in document order into a single matrix.
    if (has_matrix) rect->transform(matrix);
    scene_->add_object(rect);
}

void MitsubaLoader::parse_mesh(const XMLElement *shape_elem, bool is_ply) {
    const XMLElement *fn_elem = find_named(shape_elem, "string", "filename");
    if (!fn_elem) { LOG_ERROR("No filename specified for mesh"); return; }
    std::string filename = get_str(fn_elem);

    bool has_transform = false;
    bool has_matrix = false;
    Mat4 matrix;

    const XMLElement *xform = shape_elem->FirstChildElement("transform");
    parse_transform(xform, has_transform, nullptr, nullptr, nullptr, &has_matrix, &matrix);

    Material *mat = resolve_shape_material(shape_elem);

    Mesh *mesh = is_ply ? static_cast<Mesh *>(new PlyMesh(base_dir_ + filename, mat))
                        : static_cast<Mesh *>(new ObjMesh(base_dir_ + filename, mat));

    // parse_transform composes scale/translate/rotate/matrix in document order
    // into a single matrix, so the whole transform is applied here.
    if (has_matrix) mesh->transform(matrix);

    for (const Triangle &tri : mesh->get_triangles())
        scene_->add_object(new Triangle(tri));
    delete mesh;
}

void MitsubaLoader::parse_serialized(const XMLElement *shape_elem) {
    const XMLElement *fn_elem = find_named(shape_elem, "string", "filename");
    if (!fn_elem) { LOG_ERROR("No filename specified for serialized shape"); return; }
    std::string filename = get_str(fn_elem);

    int shape_index = 0;
    const XMLElement *si = find_named(shape_elem, "integer", "shapeIndex");
    if (!si) si = find_named(shape_elem, "integer", "shape_index");
    if (si) shape_index = get_int(si, 0);

    bool has_transform = false;
    bool has_matrix = false;
    Mat4 matrix;

    const XMLElement *xform = shape_elem->FirstChildElement("transform");
    parse_transform(xform, has_transform, nullptr, nullptr, nullptr, &has_matrix, &matrix);

    Material *mat = resolve_shape_material(shape_elem);

    Mesh *mesh = new SerializedMesh(base_dir_ + filename, shape_index, mat);

    // Composed in document order by parse_transform.
    if (has_matrix) mesh->transform(matrix);

    for (const Triangle &tri : mesh->get_triangles())
        scene_->add_object(new Triangle(tri));
    delete mesh;
}

// Mitsuba's <shape type="cube"> is an axis-aligned cube spanning [-1,1] on each
// axis, positioned by its toWorld transform. We emit it as 12 triangles with
// flat (per-face) geometric normals.
void MitsubaLoader::parse_cube(const XMLElement *shape_elem) {
    bool has_transform = false;
    bool has_matrix = false;
    Mat4 matrix;

    const XMLElement *xform = shape_elem->FirstChildElement("transform");
    parse_transform(xform, has_transform, nullptr, nullptr, nullptr, &has_matrix, &matrix);

    Material *mat = resolve_shape_material(shape_elem);

    // 8 corners of the [-1,1]^3 cube.
    const Point3 c[8] = {
        {-1,-1,-1}, {-1,-1, 1}, {-1, 1,-1}, {-1, 1, 1},
        { 1,-1,-1}, { 1,-1, 1}, { 1, 1,-1}, { 1, 1, 1},
    };
    // Six faces, each as a CCW quad (a,b,c,d) viewed from outside + its normal.
    struct Face { int a, b, c, d; Vec3 n; };
    const Face faces[6] = {
        {4,6,7,5, { 1, 0, 0}}, // +X
        {1,3,2,0, {-1, 0, 0}}, // -X
        {2,3,7,6, { 0, 1, 0}}, // +Y
        {0,4,5,1, { 0,-1, 0}}, // -Y
        {5,7,3,1, { 0, 0, 1}}, // +Z
        {0,2,6,4, { 0, 0,-1}}, // -Z
    };
    for (const Face &f : faces) {
        Triangle t0(c[f.a], c[f.b], c[f.c], f.n, f.n, f.n, mat);
        Triangle t1(c[f.a], c[f.c], c[f.d], f.n, f.n, f.n, mat);
        if (has_matrix) { t0.transform(matrix); t1.transform(matrix); }
        scene_->add_object(new Triangle(t0));
        scene_->add_object(new Triangle(t1));
    }
}

void MitsubaLoader::parse_sensor(const XMLElement *elem) {
    const char *stype = elem->Attribute("type");
    if (!stype || strcmp(stype, "perspective") != 0) {
        LOG_ERROR("Invalid camera parameter");
        return;
    }

    float fov = 45.f, aperture = 0.f, focal_dist = 1.f;
    int width = 640, height = 480;
    Vec3 lookfrom(0, 0, 5), lookat_pt(0, 0, 0), vup(0, 1, 0);

    const XMLElement *fov_elem = find_named(elem, "float", "fov");
    if (fov_elem) fov = get_float(fov_elem, 45.f);

    const XMLElement *ap = find_named(elem, "float", "apertureRadius");
    if (!ap) ap = find_named(elem, "float", "aperture");
    if (ap) aperture = get_float(ap, 0.f);

    const XMLElement *fd = find_named(elem, "float", "focusDistance");
    if (!fd) fd = find_named(elem, "float", "focalDistance");
    if (fd) focal_dist = get_float(fd, 1.f);

    const XMLElement *xform = find_named(elem, "transform", "toWorld");
    if (!xform) xform = find_named(elem, "transform", "to_world");
    if (xform) {
        bool dummy_tf = false;
        bool cam_has_matrix = false;
        Mat4 cam_matrix;
        parse_transform(xform, dummy_tf, &lookfrom, &lookat_pt, &vup,
                        &cam_has_matrix, &cam_matrix);
        // Camera given as a toWorld matrix (no lookAt): Mitsuba's camera
        // looks down local +Z with +Y up. Decompose into lookfrom/at/up.
        if (cam_has_matrix) {
            const float *cm = cam_matrix.m;
            lookfrom  = Vec3(cm[3], cm[7], cm[11]);
            Vec3 fwd(cm[2], cm[6], cm[10]);
            vup       = Vec3(cm[1], cm[5], cm[9]);
            lookat_pt = lookfrom + fwd;
        }
    }

    const XMLElement *sampler = elem->FirstChildElement("sampler");
    if (sampler) {
        const XMLElement *sc = find_named(sampler, "integer", "sampleCount");
        if (!sc) sc = find_named(sampler, "integer", "sample_count");
        if (sc) scene_->set_sample_count(get_int(sc, 1));
    }

    const XMLElement *film = elem->FirstChildElement("film");
    if (film) {
        const XMLElement *w = find_named(film, "integer", "width");
        const XMLElement *h = find_named(film, "integer", "height");
        if (w) width  = get_int(w, 640);
        if (h) height = get_int(h, 480);
    }

    float aspect = (float)width / (float)height;
    scene_->set_camera(std::make_unique<Camera>(lookfrom, lookat_pt, vup, fov,
                                                width, aspect, aperture, focal_dist));
}

void MitsubaLoader::substitute_defaults(XMLElement *elem) {
    std::vector<std::pair<std::string, std::string>> changes;
    for (const XMLAttribute *a = elem->FirstAttribute(); a; a = a->Next()) {
        std::string v = a->Value();
        std::string nv = substitute_params(v, defaults_);
        if (nv != v) changes.emplace_back(a->Name(), std::move(nv));
    }
    for (const auto &c : changes) elem->SetAttribute(c.first.c_str(), c.second.c_str());
    for (XMLElement *c = elem->FirstChildElement(); c; c = c->NextSiblingElement())
        substitute_defaults(c);
}

std::unique_ptr<Scene> MitsubaLoader::load(const std::string &filepath) {
    XMLDocument doc;
    if (doc.LoadFile(filepath.c_str()) != XML_SUCCESS) {
        LOG_ERROR("Failed to open/parse file: " + filepath);
        return nullptr;
    }

    XMLElement *scene_elem = doc.FirstChildElement("scene");
    if (!scene_elem) {
        LOG_ERROR("No <scene> root element in: " + filepath);
        return nullptr;
    }

    size_t last_slash = filepath.find_last_of("/\\");
    if (last_slash != std::string::npos)
        base_dir_ = filepath.substr(0, last_slash + 1);

    auto scene_owner = std::make_unique<Scene>();
    scene_ = scene_owner.get();

    // Mitsuba <default name=".." value=".."> template parameters: collect them,
    // then substitute every $name / ${name} reference throughout the document.
    for (auto *d = scene_elem->FirstChildElement("default"); d;
         d = d->NextSiblingElement("default")) {
        const char *nm = d->Attribute("name");
        const char *vl = d->Attribute("value");
        if (nm && vl) defaults_[nm] = vl;
    }
    if (!defaults_.empty()) substitute_defaults(scene_elem);

    for (auto *elem = scene_elem->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        const char *tag = elem->Name();

        if (strcmp(tag, "background") == 0) {
            const XMLElement *rad = find_named(elem, "rgb", "radiance");
            if (rad) scene_->set_background_color(parse_rgb(rad));

        } else if (strcmp(tag, "emitter") == 0) {
            const char *etype = elem->Attribute("type");
            if (etype && strcmp(etype, "envmap") == 0) {
                const XMLElement *fn = find_named(elem, "string", "filename");
                float escale = get_float(find_named(elem, "float", "scale"), 1.0f);
                if (fn) {
                    auto env = std::make_unique<EnvMap>(base_dir_ + get_str(fn), escale);
                    // Apply the to_world rotation (env-local -> world).
                    const XMLElement *xf = find_named(elem, "transform", "to_world");
                    if (!xf) xf = find_named(elem, "transform", "toWorld");
                    bool dummy = false, has_m = false;
                    Mat4 R;
                    parse_transform(xf, dummy, nullptr, nullptr, nullptr, &has_m, &R);
                    if (has_m) {
                        const float *m = R.m;
                        env->set_rotation(Vec3(m[0], m[4], m[8]),
                                          Vec3(m[1], m[5], m[9]),
                                          Vec3(m[2], m[6], m[10]));
                    }
                    scene_->set_env_map(std::move(env));
                }
            } else if (etype && strcmp(etype, "point") == 0) {
                Vec3 pos(0, 0, 0), intensity(1, 1, 1);
                const XMLElement *p = find_named(elem, "point", "position");
                if (p) pos = parse_xyz(p);
                const XMLElement *i = find_named(elem, "rgb", "intensity");
                if (i) intensity = parse_rgb(i);
                scene_->add_light(Light(pos, intensity));
            } else {
                LOG_ERROR("Invalid light format");
            }

        } else if (strcmp(tag, "sensor") == 0) {
            parse_sensor(elem);

        } else if (strcmp(tag, "bsdf") == 0) {
            const char *id_attr = elem->Attribute("id");
            Material *mat = parse_bsdf_element(elem);
            if (mat && id_attr) bsdf_map_[id_attr] = mat;

        } else if (strcmp(tag, "texture") == 0) {
            // Top-level <texture id="..." type="bitmap|checkerboard"> — for <ref> use.
            const char *id_attr = elem->Attribute("id");
            const char *ttype   = elem->Attribute("type");
            if (!id_attr || !ttype) {
                LOG_ERROR("top-level <texture> missing id or type");
                continue;
            }
            if (strcmp(ttype, "bitmap") != 0 && strcmp(ttype, "checkerboard") != 0) {
                LOG_ERROR("unsupported texture type: " + std::string(ttype));
                continue;
            }
            auto tex = make_texture(elem);
            if (!tex) { LOG_ERROR("<texture> missing filename"); continue; }
            texture_map_[id_attr] = tex;
            scene_->add_texture(tex);

        } else if (strcmp(tag, "shape") == 0) {
            const char *stype = elem->Attribute("type");
            if (!stype) { LOG_ERROR("Shape missing type attribute"); continue; }

            if      (strcmp(stype, "sphere") == 0)     parse_sphere(elem);
            else if (strcmp(stype, "rectangle") == 0)  parse_rectangle(elem);
            else if (strcmp(stype, "ply") == 0)        parse_mesh(elem, /*is_ply=*/true);
            else if (strcmp(stype, "obj") == 0)        parse_mesh(elem, /*is_ply=*/false);
            else if (strcmp(stype, "serialized") == 0) parse_serialized(elem);
            else if (strcmp(stype, "cube") == 0)       parse_cube(elem);
            else LOG_ERROR("Invalid shape type: " + std::string(stype));

        } else {
            // Skip elements kestrel doesn't use (integrator, default, etc.)
        }
    }

    return scene_owner;
}

} // namespace

std::unique_ptr<Scene> read_from_file(const std::string &filepath) {
    MitsubaLoader loader;
    return loader.load(filepath);
}

}  // namespace kestrel
