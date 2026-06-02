#include "plymesh.h"
#include "material.h"
#include "triangle.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace kestrel {

PlyMesh::PlyMesh(std::string filepath, const Material *material)
    : Mesh(material) {
  // Open file in binary mode from the start
  std::ifstream file(filepath, std::ios::binary);
  std::string line;
  int vertex_count = 0;
  int face_count = 0;
  bool binary = false;
  int vertex_properties = 0;
  bool in_vertex_element = false;

  std::vector<Point3> vertices;
  // store faces as lists of vertex indices
  std::vector<std::vector<int>> faces;
  std::vector<Vec3> normals;

  bool header = true;
  while (header && std::getline(file, line)) {
    // Remove trailing \r if present (Windows line endings)
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind("element vertex", 0) == 0) {
      std::istringstream iss(line);
      std::string element, vertex;
      iss >> element >> vertex >> vertex_count;
      in_vertex_element = true;
    } else if (line.rfind("element face", 0) == 0) {
      std::istringstream iss(line);
      std::string element, face;
      iss >> element >> face >> face_count;
      in_vertex_element = false;
    } else if (in_vertex_element && line.rfind("property float", 0) == 0) {
      vertex_properties++;
    }
    if (line == "format binary_little_endian 1.0") {
      binary = true;
    }
    if (line == "end_header") {
      header = false;
    }
  }

  if (vertex_properties < 3) {
    vertex_properties = 3;
  }
  int extra_floats = vertex_properties - 3;

  if (binary) {
    for (int i = 0; i < vertex_count; ++i) {
      float x, y, z;
      file.read(reinterpret_cast<char *>(&x), sizeof(float));
      file.read(reinterpret_cast<char *>(&y), sizeof(float));
      file.read(reinterpret_cast<char *>(&z), sizeof(float));
      // Skip extra properties (normals, texture coords, etc.)
      if (extra_floats > 0) {
        file.seekg(extra_floats * sizeof(float), std::ios::cur);
      }
      vertices.emplace_back(x, y, z);
      normals.emplace_back(0, 0, 0);
    }

    // Read faces (binary)
    for (int i = 0; i < face_count; ++i) {
      uint8_t vertex_index_count = 0;
      file.read(reinterpret_cast<char *>(&vertex_index_count), sizeof(uint8_t));
      std::vector<int> face_indices;
      face_indices.reserve(vertex_index_count);
      for (uint8_t k = 0; k < vertex_index_count; ++k) {
        uint32_t idx = 0;
        file.read(reinterpret_cast<char *>(&idx), sizeof(uint32_t));
        face_indices.push_back(static_cast<int>(idx));
      }
      if (face_indices.size() >= 3) {
        faces.push_back(std::move(face_indices));
      }
    }
  } else {
    for (int i = 0; i < vertex_count; ++i) {
      if (std::getline(file, line)) {
        std::istringstream iss(line);
        float x, y, z;
        iss >> x >> y >> z;
        vertices.emplace_back(x, y, z);
        normals.emplace_back(0, 0, 0);
      }
    }

    for (int i = 0; i < face_count; ++i) {
      if (std::getline(file, line)) {
        std::istringstream iss(line);
        int vertex_index_count;
        iss >> vertex_index_count;
        std::vector<int> face_indices;
        face_indices.reserve(std::max(0, vertex_index_count));
        for (int k = 0; k < vertex_index_count; ++k) {
          int idx;
          iss >> idx;
          face_indices.push_back(idx);
        }
        if (face_indices.size() >= 3) {
          faces.push_back(std::move(face_indices));
        }
      }
    }
  }
  // First pass: accumulate face normals into each vertex normal using triangle fans per face
  for (const auto &face : faces) {
    int v0 = face[0];
    if (v0 < 0 || v0 >= static_cast<int>(vertices.size())) continue;
    v0 = v0;
    for (size_t i = 1; i + 1 < face.size(); ++i) {
      int v1 = face[i];
      int v2 = face[i + 1];
      if (v1 < 0 || v2 < 0) continue;
      if (v0 >= static_cast<int>(vertices.size()) || v1 >= static_cast<int>(vertices.size()) || v2 >= static_cast<int>(vertices.size())) continue;

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
  for (const auto &face : faces) {
    int v0 = face[0];
    if (v0 < 0 || v0 >= static_cast<int>(vertices.size())) continue;
    for (size_t i = 1; i + 1 < face.size(); ++i) {
      int v1 = face[i];
      int v2 = face[i + 1];
      if (v1 < 0 || v2 < 0) continue;
      if (v1 >= static_cast<int>(vertices.size()) || v2 >= static_cast<int>(vertices.size())) continue;

      triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2],
                             normals[v0], normals[v1], normals[v2], material);
    }
  }

  file.close();
  compute_bounds();
}


}  // namespace kestrel
