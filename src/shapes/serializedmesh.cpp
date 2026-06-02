#include "serializedmesh.h"
#include "logger.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <zlib.h>

namespace kestrel {

namespace {

constexpr uint32_t FLAG_HAS_NORMALS     = 0x0001;
constexpr uint32_t FLAG_HAS_TEXCOORDS   = 0x0002;
constexpr uint32_t FLAG_HAS_TANGENTS    = 0x0004;  // deprecated, may appear
constexpr uint32_t FLAG_HAS_COLORS      = 0x0008;
constexpr uint32_t FLAG_FACE_NORMALS    = 0x0010;
constexpr uint32_t FLAG_SINGLE_PRECISION = 0x1000;
constexpr uint32_t FLAG_DOUBLE_PRECISION = 0x2000;

constexpr uint16_t FILE_MAGIC = 0x041C;

struct Cursor {
    const uint8_t *p;
    const uint8_t *end;

    bool ok() const { return p <= end; }

    template <typename T> bool read(T &out) {
        if (p + sizeof(T) > end) return false;
        std::memcpy(&out, p, sizeof(T));
        p += sizeof(T);
        return true;
    }

    bool skip_cstring() {  // null-terminated UTF-8 string (V4)
        while (p < end && *p != 0) ++p;
        if (p >= end) return false;
        ++p;
        return true;
    }
};

// Inflate `[src, src+src_len)` into `out`. Returns true on success.
static bool inflate_block(const uint8_t *src, size_t src_len,
                          std::vector<uint8_t> &out) {
    z_stream zs{};
    if (inflateInit(&zs) != Z_OK) return false;

    zs.next_in  = const_cast<Bytef *>(src);
    zs.avail_in = static_cast<uInt>(src_len);

    out.clear();
    constexpr size_t CHUNK = 64 * 1024;
    std::vector<uint8_t> buf(CHUNK);

    int ret;
    do {
        zs.next_out  = buf.data();
        zs.avail_out = static_cast<uInt>(buf.size());
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            return false;
        }
        out.insert(out.end(), buf.data(), buf.data() + (CHUNK - zs.avail_out));
    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);
    return true;
}

}  // namespace

SerializedMesh::SerializedMesh(std::string filepath, int shape_index,
                               const Material *material)
    : Mesh(material) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_ERROR("SerializedMesh: cannot open " + filepath);
        return;
    }
    const std::streamoff filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(filesize));
    file.read(reinterpret_cast<char *>(data.data()), filesize);
    file.close();

    if (data.size() < 8) {
        LOG_ERROR("SerializedMesh: file too short: " + filepath);
        return;
    }

    uint16_t magic, version;
    std::memcpy(&magic,   data.data() + 0, 2);
    std::memcpy(&version, data.data() + 2, 2);
    if (magic != FILE_MAGIC) {
        LOG_ERROR("SerializedMesh: bad magic in " + filepath);
        return;
    }
    if (version != 3 && version != 4) {
        LOG_ERROR("SerializedMesh: unsupported version " +
                  std::to_string(version) + " in " + filepath);
        return;
    }

    // Trailer: ... [N offsets] [uint32 N]
    uint32_t n_shapes;
    std::memcpy(&n_shapes, data.data() + data.size() - 4, 4);
    if (shape_index < 0 || static_cast<uint32_t>(shape_index) >= n_shapes) {
        LOG_ERROR("SerializedMesh: shapeIndex " + std::to_string(shape_index) +
                  " out of range (" + std::to_string(n_shapes) + ") in " + filepath);
        return;
    }

    const size_t offset_size = (version == 4) ? 8 : 4;
    const size_t table_start = data.size() - 4 - n_shapes * offset_size;

    auto read_offset = [&](uint32_t i) -> size_t {
        const uint8_t *p = data.data() + table_start + i * offset_size;
        if (offset_size == 8) {
            uint64_t v; std::memcpy(&v, p, 8); return static_cast<size_t>(v);
        }
        uint32_t v; std::memcpy(&v, p, 4); return static_cast<size_t>(v);
    };

    const size_t mesh_start = read_offset(static_cast<uint32_t>(shape_index));
    const size_t mesh_end   = (static_cast<uint32_t>(shape_index) + 1 < n_shapes)
                                  ? read_offset(shape_index + 1)
                                  : table_start;
    if (mesh_start >= mesh_end || mesh_end > data.size()) {
        LOG_ERROR("SerializedMesh: invalid offset table in " + filepath);
        return;
    }

    // Both V3 and V4 prefix each mesh block with the 4-byte file header
    // (magic+version). Skip it when present.
    size_t zlib_off = mesh_start;
    if (mesh_end - mesh_start > 4) {
        uint16_t m;
        std::memcpy(&m, data.data() + mesh_start, 2);
        if (m == FILE_MAGIC) zlib_off += 4;
    }

    std::vector<uint8_t> raw;
    if (!inflate_block(data.data() + zlib_off, mesh_end - zlib_off, raw)) {
        LOG_ERROR("SerializedMesh: zlib inflate failed for shapeIndex " +
                  std::to_string(shape_index) + " in " + filepath);
        return;
    }

    Cursor cur{raw.data(), raw.data() + raw.size()};

    uint32_t flags = 0;
    if (!cur.read(flags)) { LOG_ERROR("SerializedMesh: truncated flags"); return; }

    if (version == 4) {
        if (!cur.skip_cstring()) { LOG_ERROR("SerializedMesh: bad name string"); return; }
    }

    uint64_t vertex_count = 0, triangle_count = 0;
    if (!cur.read(vertex_count) || !cur.read(triangle_count)) {
        LOG_ERROR("SerializedMesh: truncated counts");
        return;
    }

    const bool double_precision = (flags & FLAG_DOUBLE_PRECISION) != 0;
    auto read_scalar = [&](float &out) -> bool {
        if (double_precision) {
            double d; if (!cur.read(d)) return false;
            out = static_cast<float>(d); return true;
        }
        return cur.read(out);
    };

    std::vector<float> positions(vertex_count * 3);
    for (uint64_t i = 0; i < vertex_count * 3; ++i) {
        if (!read_scalar(positions[i])) {
            LOG_ERROR("SerializedMesh: truncated positions"); return;
        }
    }

    const bool has_normals = (flags & FLAG_HAS_NORMALS) != 0;
    std::vector<float> normals;
    if (has_normals) {
        normals.resize(vertex_count * 3);
        for (uint64_t i = 0; i < vertex_count * 3; ++i) {
            if (!read_scalar(normals[i])) {
                LOG_ERROR("SerializedMesh: truncated normals"); return;
            }
        }
    }

    std::vector<float> uvs;
    if (flags & FLAG_HAS_TEXCOORDS) {
        uvs.resize(vertex_count * 2);
        for (uint64_t i = 0; i < vertex_count * 2; ++i) {
            if (!read_scalar(uvs[i])) {
                LOG_ERROR("SerializedMesh: truncated texcoords"); return;
            }
        }
    }

    if (flags & FLAG_HAS_COLORS) {
        for (uint64_t i = 0; i < vertex_count * 3; ++i) {
            float dummy; if (!read_scalar(dummy)) {
                LOG_ERROR("SerializedMesh: truncated colors"); return;
            }
        }
    }

    std::vector<uint32_t> indices(triangle_count * 3);
    for (uint64_t i = 0; i < triangle_count * 3; ++i) {
        if (!cur.read(indices[i])) {
            LOG_ERROR("SerializedMesh: truncated indices"); return;
        }
    }

    // Per-vertex normals: prefer the file's stored normals (Mitsuba uses them), and
    // only synthesize smooth normals from geometry when the file has none. Object-space
    // normals here; the inverse-transpose is applied later in Mesh::transform().
    std::vector<Vec3> vnormals(vertex_count, Vec3(0, 0, 0));
    auto get_pos = [&](uint32_t i) {
        return Point3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
    };
    if (has_normals) {
        for (uint64_t i = 0; i < vertex_count; ++i) {
            Vec3 n(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
            vnormals[i] = (n.length_squared() > 0.f) ? n.normalized() : n;
        }
    } else {
        for (uint64_t t = 0; t < triangle_count; ++t) {
            uint32_t i0 = indices[t * 3], i1 = indices[t * 3 + 1], i2 = indices[t * 3 + 2];
            Point3 p0 = get_pos(i0), p1 = get_pos(i1), p2 = get_pos(i2);
            Vec3 fn = Vec3::cross(p1 - p0, p2 - p0);
            if (fn.length_squared() == 0.f) continue;
            fn = fn.normalized();
            vnormals[i0] = vnormals[i0] + fn;
            vnormals[i1] = vnormals[i1] + fn;
            vnormals[i2] = vnormals[i2] + fn;
        }
        for (auto &n : vnormals)
            if (n.length_squared() > 0.f) n = n.normalized();
    }

    triangles.reserve(triangle_count);
    for (uint64_t t = 0; t < triangle_count; ++t) {
        uint32_t i0 = indices[t * 3], i1 = indices[t * 3 + 1], i2 = indices[t * 3 + 2];
        float tu0 = 0, tv0 = 0, tu1 = 1, tv1 = 0, tu2 = 0, tv2 = 1;
        if (!uvs.empty()) {
            tu0 = uvs[i0 * 2]; tv0 = uvs[i0 * 2 + 1];
            tu1 = uvs[i1 * 2]; tv1 = uvs[i1 * 2 + 1];
            tu2 = uvs[i2 * 2]; tv2 = uvs[i2 * 2 + 1];
        }
        triangles.emplace_back(get_pos(i0), get_pos(i1), get_pos(i2),
                               vnormals[i0], vnormals[i1], vnormals[i2],
                               material,
                               tu0, tv0, tu1, tv1, tu2, tv2);
    }

    compute_bounds();
}

}  // namespace kestrel
