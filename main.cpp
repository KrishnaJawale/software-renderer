#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <cmath>
#include <vector>
#include <limits>

constexpr int width  = 2048;
constexpr int height = 2048;

constexpr TGAColor white  = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green  = {  0, 255,   0, 255};
constexpr TGAColor red    = {  0,   0, 255, 255};
constexpr TGAColor blue   = {255, 128,  64, 255};
constexpr TGAColor yellow = {  0, 200, 255, 255};

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    // if line is steep, transpose coordinates
    bool steep = false;
    if (std::abs(y1 - y0) > std::abs(x1 - x0)) {
        steep = true;
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    // swap coordinates if x0 is greater than x1 (keep left to right order)
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    // calculate slope
    int y = y0;
    int error = 0;

    for (int x = x0; x <= x1; x++) {
        // if line is steep, transpose coordinates back to original
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }

        // update error and y position based on slope
        error += 2 * std::abs(y1 - y0);
        if (error > x1 - x0) {
            if (y1 > y0) {
                y += 1;
            } else {
                y -= 1;
            }

            error -= 2 * (x1 - x0);
        }
    }
}

// calculate signed area of triangle
double signed_area(int x0, int y0, int x1, int y1, int x2, int y2) {
    return (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1)) * 0.5;
}

// draw a triangle
void triangle(int x0, int y0, float z0, int x1, int y1, float z1, int x2, int y2, float z2, std::vector<float> &depthbuffer, TGAImage &framebuffer, TGAColor color) {
    // find bounding box of triangle
    int xmin = std::min({x0, x1, x2});
    int ymin = std::min({y0, y1, y2});
    int xmax = std::max({x0, x1, x2});
    int ymax = std::max({y0, y1, y2});

    // clamp bounding box to screen size (perspective function can cause values outside of screen bounds)
    xmin = std::max(0, xmin);
    ymin = std::max(0, ymin);
    xmax = std::min(width - 1, xmax);
    ymax = std::min(height - 1, ymax);
    if (xmin > xmax || ymin > ymax) return;

    // calculate signed area of triangle
    double area = signed_area(x0, y0, x1, y1, x2, y2);
    if (area < 1) return;

    // iterate over bounding box
    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            // calculate weights (barycentric coordinates)
            double w0 = signed_area(x, y, x1, y1, x2, y2) / area;
            double w1 = signed_area(x0, y0, x, y, x2, y2) / area;
            double w2 = signed_area(x0, y0, x1, y1, x, y) / area;

            // if all weights are positive, point is inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                float depth = w0 * z0 + w1 * z1 + w2 * z2;
                int idx = x + y * width;
                if (depthbuffer[idx] < depth) {
                    framebuffer.set(x, y, color);
                    depthbuffer[idx] = depth;
                }
            }
        }
    }
}

// rotate
Vec3f rotate(Vec3f v) {
    constexpr float angle = M_PI / 6;
    const Mat3f Ry = {
        Vec3f(std::cos(angle), 0.f, std::sin(angle)),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(-std::sin(angle), 0.f, std::cos(angle))
    };
    return Ry * v;
}

// perspective projection
Vec3f perspective(Vec3f v) {
    constexpr float d = 3.0f;
    return v / (1.0f - (v.z / d));
}

// viewpoint transformation
std::tuple<int, int, float> project(Vec3f v) {
    return {
        (v.x + 1.0f) * width * 0.5f,
        (v.y + 1.0f) * height * 0.5f,
        v.z,
    };
}

int main(int argc, char** argv) {
    // load model
    Model model("models/diablo3.obj");

    TGAImage framebuffer(width, height, TGAImage::RGB);
    // depth buffer to keep track of the depth of each pixel
    std::vector<float> depthbuffer(width * height, -std::numeric_limits<float>::infinity());

    // draw faces
    for (const auto& face : model.faces) {
        // for each face, get the 3 corresponding vertices and project them to the screen
        auto [x0, y0, z0] = project(perspective(rotate(model.verts[face.x])));
        auto [x1, y1, z1] = project(perspective(rotate(model.verts[face.y])));
        auto [x2, y2, z2] = project(perspective(rotate(model.verts[face.z])));

        TGAColor rand_color;
        for (int c = 0; c < 3; c++) {
            rand_color[c] = std::rand() % 255;
        }

        triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, depthbuffer, framebuffer, rand_color);
    }

    framebuffer.write_tga_file("framebuffer.tga");

    // depth buffer TGA image for debugging
    TGAImage depthimg(width, height, TGAImage::GRAYSCALE);

    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();

    for (float depth : depthbuffer) {
        if (depth == -std::numeric_limits<float>::infinity()) continue;
        
        if (depth > zmax) zmax = depth;
        if (depth < zmin) zmin = depth;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width;
            float z = depthbuffer[idx];

            if (z == -std::numeric_limits<float>::infinity()) {
                depthimg.set(x, y, {0});
                continue;
            }

            float normalized_z = (z - zmin) / (zmax - zmin);
            unsigned char depth = static_cast<unsigned char>(normalized_z * 255.0f);
            depthimg.set(x, y, {depth});
        }
    }

    depthimg.write_tga_file("depthimg.tga");

    return 0;
}
