#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include "geometry.h"

class Model {
    public:
        std::vector<Vec3f> verts;
        std::vector<Vec2f> tex_coords;
        std::vector<Vec3i> faces;
        std::vector<Vec3i> face_tex_coords;
        float scale;

        Model(const std::string &filename)
            : verts(), tex_coords(), faces(), face_tex_coords(), scale(1.0f) {
            std::ifstream in(filename);
            if (!in) {
                std::cerr << "can't open " << filename << "\n";
                return;
            }

            std::string line;

            // initialize bounding box and related variables (for scaling)
            float min_x = std::numeric_limits<float>::max();
            float min_y = std::numeric_limits<float>::max();
            float min_z = std::numeric_limits<float>::max();
            float max_x = std::numeric_limits<float>::lowest();
            float max_y = std::numeric_limits<float>::lowest();
            float max_z = std::numeric_limits<float>::lowest();
            float center_x, center_y, center_z;
            
            while (std::getline(in, line)) {
                std::istringstream iss(line);
                std::string prefix;
                iss >> prefix;
                if (prefix == "v") {
                    // read vertex coordinates
                    Vec3f vert;
                    iss >> vert.x >> vert.y >> vert.z;
                    verts.push_back(vert);

                    // update bounding box
                    min_x = std::min(min_x, vert.x);
                    max_x = std::max(max_x, vert.x);
                    min_y = std::min(min_y, vert.y);
                    max_y = std::max(max_y, vert.y);
                    min_z = std::min(min_z, vert.z);
                    max_z = std::max(max_z, vert.z);
                } else if (prefix == "vt") {
                    Vec2f tex_coord;
                    iss >> tex_coord.x >> tex_coord.y;
                    tex_coords.push_back(tex_coord);
                } else if (prefix == "f") {
                    // face format is v, v/vt, v//vn, or v/vt/vn (obj files are 1 indexed)
                    Vec3i face;
                    Vec3i face_tex;
                    for (int corner = 0; corner < 3; corner++) {
                        std::string token;
                        iss >> token;
                        int vert_index = 0;
                        int tex_index = -1;
                        parse_face_token(token, vert_index, tex_index);
                        if (corner == 0) {
                            face.x = vert_index;
                            face_tex.x = tex_index;
                        } else if (corner == 1) {
                            face.y = vert_index;
                            face_tex.y = tex_index;
                        } else {
                            face.z = vert_index;
                            face_tex.z = tex_index;
                        }
                    }

                    faces.push_back(face);
                    face_tex_coords.push_back(face_tex);
                }
            }

            // calculate scale and center of the model
            float model_width = max_x - min_x;
            float model_height = max_y - min_y;
            scale = std::max(model_width, model_height) / 2.0f;
            center_x = (min_x + max_x) / 2.0f;
            center_y = (min_y + max_y) / 2.0f;
            center_z = (min_z + max_z) / 2.0f;

            // scale vertices to the center of the model, normalized to the range [-1, 1]
            for (auto& vert : verts) {
                vert.x = (vert.x - center_x) / scale;
                vert.y = (vert.y - center_y) / scale;
                vert.z = (vert.z - center_z) / scale;
            }
        }

    private:
        // Parse 1 face corner into 0 based vertex and texture indices
        // Normal index (3rd field) ignored for now
        static void parse_face_token(const std::string &token, int &vert_index, int &tex_index) {
            const size_t first_slash = token.find('/');
            if (first_slash == std::string::npos) {
                vert_index = std::stoi(token) - 1; // -1 because obj files are 1 indexed
                tex_index = -1;
                return;
            }
            vert_index = std::stoi(token.substr(0, first_slash)) - 1;
            const size_t second_slash = token.find('/', first_slash + 1);
            const std::string tex_token = (second_slash == std::string::npos)
                ? token.substr(first_slash + 1)
                : token.substr(first_slash + 1, second_slash - first_slash - 1);
            tex_index = tex_token.empty() ? -1 : std::stoi(tex_token) - 1;
        }
};
