#include "objmesh.h"
#include "material.h"
#include "triangle.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace kestrel {

ObjMesh::ObjMesh(std::string filepath, const Material *material)
    : Mesh(material) {

  std::ifstream file(filepath);
  std::string line;
  int vertex_count = 0;
  int face_count = 0;

  std::vector<Point3> vertices;
  std::vector<std::vector<int>> faces;
  std::vector<std::vector<int>> face_uvs; // per-face per-vertex UV index (0-based, -1 = none)
  std::vector<Vec3> normals;
  std::vector<std::pair<float,float>> uvs; // vt entries

  while (std::getline(file, line)) {
    // Parse 'v ' vertex positions
    if (line.size() > 1 && line[0] == 'v' && std::isspace((unsigned char)line[1])) {
      float x, y, z;
      std::istringstream iss(line.substr(1));
      iss >> x >> y >> z;
      vertices.emplace_back(x, y, z);
      normals.emplace_back(0, 0, 0);
    }

    // Parse 'vt' texture coordinates
    if (line.size() > 2 && line[0] == 'v' && line[1] == 't' && std::isspace((unsigned char)line[2])) {
      float u, v;
      std::istringstream iss(line.substr(2));
      iss >> u >> v;
      uvs.emplace_back(u, v);
    }

    // Parse 'f ' face lines — token format: v, v/vt, or v/vt/vn (1-based)
    if (line.size() > 1 && line[0] == 'f' && std::isspace((unsigned char)line[1])) {
      std::istringstream iss(line.substr(1));
      std::string token;
      std::vector<int> face_indices;
      std::vector<int> uv_indices;

      while (iss >> token) {
        size_t slash1 = token.find('/');
        std::string vidx_str = (slash1 == std::string::npos) ? token : token.substr(0, slash1);
        try {
          face_indices.push_back(std::stoi(vidx_str));
        } catch (...) { continue; }

        int uvi = -1;
        if (slash1 != std::string::npos && slash1 + 1 < token.size() && token[slash1+1] != '/') {
          size_t slash2 = token.find('/', slash1 + 1);
          std::string uv_str = token.substr(slash1 + 1, slash2 == std::string::npos ? std::string::npos : slash2 - slash1 - 1);
          try { uvi = std::stoi(uv_str) - 1; } catch (...) {}
        }
        uv_indices.push_back(uvi);
      }

      if (face_indices.size() >= 3) {
        faces.push_back(std::move(face_indices));
        face_uvs.push_back(std::move(uv_indices));
      }
    }
  }
  // First pass: accumulate face normals into each vertex normal using triangle fans per face
  for (const auto &face : faces) {
    // triangle fan: (v0, vi, vi+1)
    int v0 = face[0] - 1;
    if (v0 < 0 || v0 >= static_cast<int>(vertices.size())) continue;
    for (size_t i = 1; i + 1 < face.size(); ++i) {
      int v1 = face[i] - 1;
      int v2 = face[i + 1] - 1;
      if (v1 < 0 || v2 < 0) continue;
      if (v1 >= static_cast<int>(vertices.size()) || v2 >= static_cast<int>(vertices.size())) continue;

      Vec3 edge1 = vertices[v1] - vertices[v0];
      Vec3 edge2 = vertices[v2] - vertices[v0];
      Vec3 face_normal = Vec3::cross(edge1, edge2);
      if (face_normal.length_squared() == 0.0f) continue;
      face_normal = face_normal.normalized();

      normals[v0] = normals[v0] + face_normal;
      normals[v1] = normals[v1] + face_normal;
      normals[v2] = normals[v2] + face_normal;
    }
  }

  // Normalize accumulated vertex normals
  for (auto &n : normals) {
    if (n.length_squared() == 0.0f) continue;
    n = n.normalized();
  }

  // Second pass: create triangles using per-vertex shading normals (triangle fans)
  for (size_t fi = 0; fi < faces.size(); ++fi) {
    const auto &face   = faces[fi];
    const auto &faceUV = face_uvs[fi];

    int v0 = face[0] - 1;
    if (v0 < 0 || v0 >= static_cast<int>(vertices.size())) continue;
    for (size_t i = 1; i + 1 < face.size(); ++i) {
      int v1 = face[i] - 1;
      int v2 = face[i + 1] - 1;
      if (v1 < 0 || v2 < 0) continue;
      if (v1 >= static_cast<int>(vertices.size()) || v2 >= static_cast<int>(vertices.size())) continue;

      float tu0 = 0, tv0 = 0, tu1 = 1, tv1 = 0, tu2 = 0, tv2 = 1;
      if (faceUV.size() > i + 1) {
        auto get_uv = [&](int uvi, float &out_u, float &out_v) {
          if (uvi >= 0 && uvi < static_cast<int>(uvs.size())) {
            out_u = uvs[uvi].first;
            out_v = uvs[uvi].second;
          }
        };
        get_uv(faceUV[0],   tu0, tv0);
        get_uv(faceUV[i],   tu1, tv1);
        get_uv(faceUV[i+1], tu2, tv2);
      }

      triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2],
                             normals[v0], normals[v1], normals[v2], material,
                             tu0, tv0, tu1, tv1, tu2, tv2);
    }
  }
  file.close();
  compute_bounds();
}


}  // namespace kestrel
