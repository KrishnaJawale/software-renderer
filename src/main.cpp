#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <cmath>
#include <vector>
#include <limits>

Mat4f ModelView, Viewport, Projection;

constexpr TGAColor white  = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green  = {  0, 255,   0, 255};
constexpr TGAColor red    = {  0,   0, 255, 255};
constexpr TGAColor blue   = {255, 128,  64, 255};
constexpr TGAColor yellow = {  0, 200, 255, 255};

void viewport(const int x, const int y, const int w, const int h) {
    // viewport transformation matrix
    // multiplying a vec4 by this matrix transforms the point or vector to the viewport space
    Viewport = {
        {w / 2.0f, 0.0f, 0.0f, x + w / 2.0f},
        {0.0f, h / 2.0f, 0.0f, y + h / 2.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
}

void perspective(const double focal_length) {
    // perspective transformation matrix
    // multiplying a vec4 by this matrix transforms the point or vector to the perspective space
    Projection = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0, static_cast<float>(-1/focal_length),1}
    };
}

void lookat(const Vec3f &eye, const Vec3f &center, const Vec3f &up) {
    // lookat transformation matrix
    // multiplying a vec4 by this matrix transforms the point or vector to the lookat space
    Vec3f to_camera = normalize(eye - center); // vector from what we are looking to the camera
    Vec3f right = normalize(cross(up, to_camera)); // vector to the right of the camera
    Vec3f camera_up = normalize(cross(to_camera, right)); // vector up from the camera
    ModelView = Mat4f({
        {right.x, right.y, right.z, 0},
        {camera_up.x, camera_up.y, camera_up.z, 0},
        {to_camera.x, to_camera.y, to_camera.z, 0},
        {0, 0, 0, 1}
    }) * Mat4f({
        {1, 0, 0, -center.x},
        {0, 1, 0, -center.y},
        {0, 0, 1, -center.z},
        {0, 0, 0, 1}
    });
}

// calculate signed area of triangle
double signed_area(int x0, int y0, int x1, int y1, int x2, int y2) {
    return (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1)) * 0.5;
}

// rasterize
void triangle(int x0, int y0, float z0, int x1, int y1, float z1, int x2, int y2, float z2, int width, int height, std::vector<float> &depthbuffer, TGAImage &framebuffer, TGAColor color) {
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
    // Degenerate/very small triangles: avoid division by close to 0 and numerical noise
    if (std::abs(area) < 1e-8) return;

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

int main(int argc, char** argv) {
    // set screen size and camera parameters
    constexpr int width  = 2048;
    constexpr int height = 2048;
    const Vec3f eye{-1.f,0.f,2.f};
    const Vec3f center{0.f,0.f,0.f};
    const Vec3f up{0.f,1.f,0.f};

    // set viewport and perspective transformation matrices
    viewport(width/16, height/16, width * 7/8, height * 7/8);
    perspective(norm(eye-center));
    lookat(eye, center, up);

    // load model
    Model model("models/diablo3.obj");

    TGAImage framebuffer(width, height, TGAImage::RGB);
    // depth buffer to keep track of the depth of each pixel
    std::vector<float> depthbuffer(width * height, -std::numeric_limits<float>::infinity());

    // draw faces
    for (const auto& face : model.faces) {
        Vec3f verts[3] = {
            model.verts[face.x],
            model.verts[face.y],
            model.verts[face.z]
        };
        int screen_x[3], screen_y[3];
        float depth[3];
        for (int i = 0; i < 3; i++) {
            Vec4f clip = Projection * ModelView * Vec4f(verts[i].x, verts[i].y, verts[i].z, 1.f);
            Vec4f ndc  = clip / clip.w;
            Vec4f screen  = Viewport * ndc;
            screen_x[i] = static_cast<int>(screen.x);
            screen_y[i] = static_cast<int>(screen.y);
            depth[i] = ndc.z;
        }

        TGAColor color;
        for (int c = 0; c < 3; c++) {
            color[c] = std::rand() % 255;
        }

        triangle(screen_x[0], screen_y[0], depth[0], screen_x[1], screen_y[1], depth[1], screen_x[2], screen_y[2], depth[2], width, height, depthbuffer, framebuffer, color);
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
