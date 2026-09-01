#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <cmath>

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
void triangle(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, TGAImage &depthbuffer, TGAImage &framebuffer, TGAColor color) {
    // find bounding box of triangle
    int xmin = std::min({x0, x1, x2});
    int ymin = std::min({y0, y1, y2});
    int xmax = std::max({x0, x1, x2});
    int ymax = std::max({y0, y1, y2});

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
                unsigned char depth = static_cast<unsigned char>(w0 * z0 + w1 * z1 + w2 * z2);
                if (depthbuffer.get(x, y)[0] < depth) {
                    framebuffer.set(x, y, color);
                    depthbuffer.set(x, y, {depth});
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
std::tuple<int, int, int> project(Vec3f v) {
    return {
        (v.x + 1.0f) * width * 0.5f,
        (v.y + 1.0f) * height * 0.5f,
        (v.z + 1.0f) * 255. * 0.5f,
    };
}

int main(int argc, char** argv) {
    // load model
    Model model("models/diablo3.obj");

    TGAImage framebuffer(width, height, TGAImage::RGB);
    // depth buffer to keep track of the depth of each pixel
    TGAImage depthbuffer(width, height, TGAImage::GRAYSCALE);

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
    depthbuffer.write_tga_file("depthbuffer.tga");
    return 0;
}
