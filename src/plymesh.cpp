#include "plymesh.h"
#include "bsdfs/material.h"
#include "triangle.h"
#include <fstream>
#include <iostream>
#include <sstream>

PlyMesh::PlyMesh(std::string filepath, const Material *material)
    : Shape(material) {
  std::fstream file(filepath);
  std::string line;
  int vertex_count = 0;
  int face_count = 0;

  bool header = true;
  while (header && std::getline(file, line)) {
    if (line.rfind("element vertex", 0) == 0) {
      // line starts with "element vertex"
      std::istringstream iss(line);
      std::string element, vertex;
      iss >> element >> vertex >> vertex_count;
    }
    if (line.rfind("element face", 0) == 0) {
      // line starts with "element face"
      std::istringstream iss(line);
      std::string element, face;
      iss >> element >> face >> face_count;
    }
    if (line == "end_header") {
      header = false;
    }
  }

  // Parse vertices
  std::vector<Point3> vertices;
  for (int i = 0; i < vertex_count; ++i) {
    if (std::getline(file, line)) {
      std::istringstream iss(line);
      float x, y, z;
      iss >> x >> y >> z;
      vertices.emplace_back(x,y,z);
    }
  }

  // Parse faces and create triangles
  for (int i = 0; i < face_count; ++i) {
    if (std::getline(file, line)) {
      std::istringstream iss(line);
      int vertex_index_count;
      iss >> vertex_index_count;

      // For triangulated faces, read indices
      if (vertex_index_count >= 3) {
        int idx0, idx1, idx2;
        iss >> idx0 >> idx1 >> idx2;
        triangles.emplace_back(vertices[idx0], vertices[idx1], vertices[idx2],
                               material);
      }
    }
  }

  std::cout << "Number of vertices: " << vertex_count << std::endl;
  std::cout << "Number of faces: " << face_count << std::endl;
  std::cout << "Number of triangles created: " << triangles.size() << std::endl;
}

bool PlyMesh::hit(const Ray &ray, float t_min, float t_max,
                  HitRecord &rec) const {
  bool hit_any = false;
  float closest_so_far = t_max;

  for (const auto &tri : triangles) {
    HitRecord temp_rec;
    if (tri.hit(ray, t_min, closest_so_far, temp_rec)) {
      hit_any = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
      rec.material = material;
    }
  }
  return hit_any;
}

void PlyMesh::scale(Vec3 factor) {
  for (auto &tri : triangles) {
    tri.scale(factor);
  }
}

void PlyMesh::translate(Vec3 offset) {
  for (auto &tri : triangles) {
    tri.translate(offset);
  }
}