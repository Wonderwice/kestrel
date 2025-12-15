#include "bsdfs/conductor.h"
#include "bsdfs/lambertian.h"
#include "kestrel.h"
#include "logger.h"
#include "plymesh.h"
#include "scene.h"
#include "sphere.h"

#include <fstream>
#include <map>

Scene *read_from_file(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    LOG_ERROR("Failed to open file: " + filepath);
    return nullptr;
  }
  Scene *scene = new Scene();
  Camera *cam = nullptr;

  // Map to store BSDFs by their ID for later reference
  std::map<std::string, Material *> bsdf_map;

  // For now, this is a placeholder that reads the file
  std::string line;

  std::getline(file, line);

  if (!(line == "<scene>")) {
    LOG_ERROR("File should start with scene !");
    file.close();
    return scene;
  }

  while (std::getline(file, line)) {
    if (line.empty())
      continue;
    if (line.find("<!--") != std::string::npos) {
      while (!(line.find("-->") != std::string::npos)) {
        std::getline(file, line);
      }
    } else if (line.find("<camera") != std::string::npos) {
      if (line.find("perspective") != std::string::npos) {
        // Parse camera parameters from subsequent lines
        float fov, aspect, width, height;
        Vec3 lookfrom, lookat, vup;

        while (!(line.find("</camera>") != std::string::npos)) {
          std::getline(file, line);
          if (line.find("fov") != std::string::npos) {
            fov =
                std::stof(line.substr(line.find("value=") + 7,
                                      line.find("\"", line.find("value=") + 7) -
                                          (line.find("value=") + 7)));
          }

          if (line.find("height") != std::string::npos) {
            height =
                std::stoi(line.substr(line.find("value=") + 7,
                                      line.find("\"", line.find("value=") + 7) -
                                          (line.find("value=") + 7)));
          }

          if (line.find("width") != std::string::npos) {
            width =
                std::stoi(line.substr(line.find("value=") + 7,
                                      line.find("\"", line.find("value=") + 7) -
                                          (line.find("value=") + 7)));
          }

          if (line.find("toWorld")) {
            if (line.find("lookat") != std::string::npos) {
              // Parse origin
              size_t origin_pos = line.find("origin=") + 8;
              size_t origin_end = line.find("\"", origin_pos);
              std::string origin_str =
                  line.substr(origin_pos, origin_end - origin_pos);

              // Parse target
              size_t target_pos = line.find("target=") + 8;
              size_t target_end = line.find("\"", target_pos);
              std::string target_str =
                  line.substr(target_pos, target_end - target_pos);

              // Parse up
              size_t up_pos = line.find("up=") + 4;
              size_t up_end = line.find("\"", up_pos);
              std::string up_str = line.substr(up_pos, up_end - up_pos);

              // Extract individual coordinates (simple parsing assuming "x, y,
              // z" format)
              float origin_x, origin_y, origin_z, target_x, target_y, target_z,
                  up_x, up_y, up_z;
              sscanf(origin_str.c_str(), "%f, %f, %f", &origin_x, &origin_y,
                     &origin_z);
              sscanf(target_str.c_str(), "%f, %f, %f", &target_x, &target_y,
                     &target_z);
              sscanf(up_str.c_str(), "%f, %f, %f", &up_x, &up_y, &up_z);

              lookfrom = Point3(origin_x, origin_y, origin_z);
              lookat = Point3(target_x, target_y, target_z);
              vup = Vec3(up_x, up_y, up_z);
            }
          }
        }
        aspect = width / height;

        cam = new Camera(lookfrom, lookat, vup, fov, width, aspect);
        scene->camera = cam;
      } else {
        std::cerr << "Invalid camera parameter" << std::endl;
      }
    } else if (line.find("<emitter") != std::string::npos) {
      if (line.find("point") != std::string::npos) {
        float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f;
        float int_r = 1.0f, int_g = 1.0f, int_b = 1.0f;

        while (!(line.find("</emitter>") != std::string::npos)) {
          std::getline(file, line);
          if (line.find("position") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            sscanf(value_str.c_str(), "%f, %f, %f", &pos_x, &pos_y, &pos_z);
          }
          if (line.find("intensity") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            sscanf(value_str.c_str(), "%f, %f, %f", &int_r, &int_g, &int_b);
          }
        }
        scene->add_light(
            Light(Vec3(pos_x, pos_y, pos_z), Vec3(int_r, int_g, int_b)));
      } else {
        std::cerr << "Invalid light format" << std::endl;
      }
    } else if (line.find("<shape") != std::string::npos) {
      std::string bsdf_ref = "";
      Material *material = nullptr;
      if (line.find("sphere") != std::string::npos) {
        // Parse sphere parameters
        float center_x = 0.0f, center_y = 0.0f, center_z = 0.0f, radius = 1.0f;

        while (!(line.find("</shape>") != std::string::npos)) {
          std::getline(file, line);
          if (line.find("center") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            sscanf(value_str.c_str(), "%f, %f, %f", &center_x, &center_y,
                   &center_z);
          }
          if (line.find("radius") != std::string::npos) {
            // Parse the value attribute
            radius =
                std::stof(line.substr(line.find("value=") + 7,
                                      line.find("\"", line.find("value=") + 7) -
                                          (line.find("value=") + 7)));
          }

          // Parse BSDF reference
          if (line.find("<ref") != std::string::npos) {
            size_t ref_pos = line.find("id=") + 4;
            size_t ref_end = line.find("\"", ref_pos);
            bsdf_ref = line.substr(ref_pos, ref_end - ref_pos);
          }
        }

        // Look up the material by reference
        if (!bsdf_ref.empty() && bsdf_map.find(bsdf_ref) != bsdf_map.end()) {
          material = bsdf_map[bsdf_ref];
        } else {
          // Fallback to default material if no valid reference
          if (!bsdf_ref.empty()) {
            std::cerr << "Warning: BSDF reference '" << bsdf_ref
                      << "' not found. " << std::endl;
          }
          std::cerr << "Using default Lambertian material" << std::endl;
          material = new Lambertian(Color(0.0f));
        }

        Sphere *sphere =
            new Sphere(Point3(center_x, center_y, center_z), radius, material);
        scene->add_object(sphere);
      } else if (line.find("ply") != std::string::npos) {
        std::string filename = "";
        std::string bsdf_ref = "";
        Vec3 scale_factor(1.0f, 1.0f, 1.0f);
        Vec3 translate_offset(0.0f, 0.0f, 0.0f);
        bool has_transform = false;

        while (!(line.find("</shape>") != std::string::npos)) {
          std::getline(file, line);

          // Parse filename - FIXED: removed negation
          if (line.find("filename") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            filename = line.substr(value_pos, value_end - value_pos);
          }

          if (line.find("<ref") != std::string::npos) {
            size_t ref_pos = line.find("id=") + 4;
            size_t ref_end = line.find("\"", ref_pos);
            bsdf_ref = line.substr(ref_pos, ref_end - ref_pos);
          }

          // Parse transform: scale
          if (line.find("<scale") != std::string::npos) {
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            float sx, sy, sz;
            sscanf(value_str.c_str(), "%f, %f, %f", &sx, &sy, &sz);
            scale_factor = Vec3(sx, sy, sz);
            has_transform = true;
          }

          // Parse transform: translate
          if (line.find("<translate") != std::string::npos) {
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            float tx, ty, tz;
            sscanf(value_str.c_str(), "%f, %f, %f", &tx, &ty, &tz);
            translate_offset = Vec3(tx, ty, tz);
            has_transform = true;
          }
        }

        // Look up the material by reference
        if (!bsdf_ref.empty() && bsdf_map.find(bsdf_ref) != bsdf_map.end()) {
          material = bsdf_map[bsdf_ref];
        } else {
          // Fallback to default material if no valid reference
          if (!bsdf_ref.empty()) {
            std::cerr << "Warning: BSDF reference '" << bsdf_ref
                      << "' not found. ";
            std::cerr << "Available BSDFs: ";
            for (const auto &pair : bsdf_map) {
              std::cerr << "'" << pair.first << "' ";
            }
            std::cerr << std::endl;
          }
          std::cerr << "Using default Lambertian material for PLY mesh"
                    << std::endl;
          material = new Lambertian(Color(0.5f, 0.5f, 0.5f));
        }

        if (filename.empty()) {
          std::cerr << "Error: No filename specified for PLY mesh" << std::endl;
        } else {
          PlyMesh *plymesh = new PlyMesh(filename, material);

          // Apply transformations if specified
          if (has_transform) {
            plymesh->scale(scale_factor);
            plymesh->translate(translate_offset);
          }

          scene->add_object(plymesh);
        }
      } else {
        while (!(line.find("</shape>") != std::string::npos)) {
          std::getline(file, line);
        }
        std::cerr << "Invalid shape" << std::endl;
      }
    } else if (line.find("<bsdf") != std::string::npos) {
      // Parse BSDF type
      std::string bsdf_id = "";

      // Check for explicit ID in the tag
      size_t id_pos = line.find("id=");
      if (id_pos != std::string::npos) {
        size_t id_start = line.find("\"", id_pos) + 1;
        size_t id_end = line.find("\"", id_start);
        bsdf_id = line.substr(id_start, id_end - id_start);
      }

      if (line.find("lambertian") != std::string::npos) {
        float r = 0.f, g = 0.f, b = 0.f;

        while (!(line.find("</bsdf>") != std::string::npos)) {
          std::getline(file, line);
          if (line.find("color") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            sscanf(value_str.c_str(), "%f, %f, %f", &r, &g, &b);
          }
        }
        Lambertian *lambertian = new Lambertian(Color(r, g, b));
        bsdf_map[bsdf_id] = lambertian;
        scene->add_bsdf(lambertian);
      } else if (line.find("conductor") != std::string::npos) {
        float eta_r = 1.0f, eta_g = 1.0f, eta_b = 1.0f;

        while (!(line.find("</bsdf>") != std::string::npos)) {
          std::getline(file, line);
          if (line.find("eta") != std::string::npos) {
            // Parse the value attribute
            size_t value_pos = line.find("value=") + 7;
            size_t value_end = line.find("\"", value_pos);
            std::string value_str =
                line.substr(value_pos, value_end - value_pos);
            sscanf(value_str.c_str(), "%f, %f, %f", &eta_r, &eta_g, &eta_b);
          }
        }
        Conductor *conductor = new Conductor(Color(eta_r, eta_g, eta_b));
        bsdf_map[bsdf_id] = conductor;
        scene->add_bsdf(conductor);
      } else {
        while (!(line.find("</bsdf>") != std::string::npos)) {
          std::getline(file, line);
        }
        std::cerr << "Invalid bsdf" << std::endl;
      }
    } else if (line.find("</scene>") != std::string::npos) {
      file.close();
      return scene;
    } else {
      std::cerr << "Unknown element in scene file" << std::endl;
      std::cerr << line << std::endl;
    }
  }

  file.close();
  return scene;
}