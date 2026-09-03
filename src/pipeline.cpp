#include "pipeline.h"
#include <algorithm>
#include <cmath>
#include <limits>

void Pipeline::lookat(const Vec3f &eye, const Vec3f &center, const Vec3f &up) {
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

void Pipeline::set_viewport(const int x, const int y, const int w, const int h) {
    // viewport transformation matrix
    // multiplying a vec4 by this matrix transforms the point or vector to the viewport space
    Viewport = {
        {w / 2.0f, 0.0f, 0.0f, x + w / 2.0f},
        {0.0f, h / 2.0f, 0.0f, y + h / 2.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
}

void Pipeline::set_perspective(const double focal_length) {
    // perspective transformation matrix
    // multiplying a vec4 by this matrix transforms the point or vector to the perspective space
    Projection = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0, static_cast<float>(-1/focal_length),1}
    };
}

void Pipeline::init_depthbuffer(const int width, const int height) {
    depthbuffer.resize(width * height, -std::numeric_limits<float>::infinity());
}

// calculate signed area of triangle
static double signed_area(int x0, int y0, int x1, int y1, int x2, int y2) {
    return (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1)) * 0.5;
}

// rasterize a triangle given in clip space; fragment color is computed by the shader
void rasterize(Pipeline &pipeline, const Vec4f clip[3], const IShader &shader, TGAImage &framebuffer) {
    int screen_x[3], screen_y[3];
    float depth[3];
    for (int i = 0; i < 3; i++) {
        Vec4f ndc = clip[i] / clip[i].w;
        Vec4f screen = pipeline.Viewport * ndc;
        screen_x[i] = static_cast<int>(screen.x);
        screen_y[i] = static_cast<int>(screen.y);
        depth[i] = ndc.z;
    }

    int x0 = screen_x[0], y0 = screen_y[0];
    int x1 = screen_x[1], y1 = screen_y[1];
    int x2 = screen_x[2], y2 = screen_y[2];
    float z0 = depth[0], z1 = depth[1], z2 = depth[2];

    int width = framebuffer.width();
    int height = framebuffer.height();

    // find bounding box of triangle
    int xmin = std::min({x0, x1, x2});
    int ymin = std::min({y0, y1, y2});
    int xmax = std::max({x0, x1, x2});
    int ymax = std::max({y0, y1, y2});

    // clamp bounding box to screen size (perspective can cause values outside of screen bounds)
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
                if (pipeline.depthbuffer[idx] < depth) {
                    Vec3f bar{w0, w1, w2};
                    auto [discard, color] = shader.fragment(bar);
                    if (discard) continue;
                    framebuffer.set(x, y, color);
                    pipeline.depthbuffer[idx] = depth;
                }
            }
        }
    }
}
