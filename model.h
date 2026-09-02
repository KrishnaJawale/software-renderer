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
        std::vector<Vec3i> faces;
        float scale;

        Model(const std::string &filename) : verts(), faces(), scale(1.0f) {
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
                    Vec3f v;
                    iss >> v.x >> v.y >> v.z;
                    verts.push_back(v);

                    // update bounding box
                    min_x = std::min(min_x, v.x);
                    max_x = std::max(max_x, v.x);
                    min_y = std::min(min_y, v.y);
                    max_y = std::max(max_y, v.y);
                    min_z = std::min(min_z, v.z);
                    max_z = std::max(max_z, v.z);
                } else if (prefix == "f") {
                    // read face indices
                    Vec3i f;
                    
                    // for each face index subtract 1 (obj files are 1 indexed)
                    std::string token;
                    iss >> token;
                    f.x = std::stoi(token) - 1;
                    iss >> token;
                    f.y = std::stoi(token) - 1;
                    iss >> token;
                    f.z = std::stoi(token) - 1;

                    faces.push_back(f);
                }
            }

            // calculate scale and center of the model
            float model_width = max_x - min_x;
            float model_height = max_y - min_y;
            float model_depth = max_z - min_z;
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
};
