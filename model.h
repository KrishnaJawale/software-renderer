#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Vec3f {
    float x, y, z;
};

struct Vec3i {
    int x, y, z;
};

class Model {
    public:
        std::vector<Vec3f> verts;
        std::vector<Vec3i> faces;

        Model(const std::string &filename) : verts(), faces() {
            std::ifstream in(filename);
            if (!in) {
                std::cerr << "can't open " << filename << "\n";
                return;
            }

            std::string line;
            while (std::getline(in, line)) {
                std::istringstream iss(line);
                std::string prefix;
                iss >> prefix;
                if (prefix == "v") {
                    // read vertex coordinates
                    Vec3f v;
                    iss >> v.x >> v.y >> v.z;
                    verts.push_back(v);
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
        }
};