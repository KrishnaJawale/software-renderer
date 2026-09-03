#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "pipeline.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <vector>

struct RandomShader : IShader {
    TGAColor color{};

    std::pair<bool, TGAColor> fragment(const Vec3f &) const override {
        return {false, color}; // do not discard, flat random color per triangle
    }
};

int main(int argc, char** argv) {
    // set screen size and camera parameters
    constexpr int width  = 2048;
    constexpr int height = 2048;
    const Vec3f eye{-1.f,0.f,2.f};
    const Vec3f center{0.f,0.f,0.f};
    const Vec3f up{0.f,1.f,0.f};

    // set up pipeline
    Pipeline pipeline;
    pipeline.set_viewport(width/16, height/16, width * 7/8, height * 7/8);
    pipeline.set_perspective(norm(eye-center));
    pipeline.lookat(eye, center, up);
    pipeline.init_depthbuffer(width, height);

    TGAImage framebuffer(width, height, TGAImage::RGB);

    // load model
    Model model("models/diablo3.obj");

    RandomShader shader;

    // draw faces
    for (const auto& face : model.faces) {
        Vec3f verts[3] = {
            model.verts[face.x],
            model.verts[face.y],
            model.verts[face.z]
        };

        Vec4f clip[3];
        for (int i = 0; i < 3; i++) {
            clip[i] = pipeline.Projection * pipeline.ModelView * Vec4f(verts[i].x, verts[i].y, verts[i].z, 1.f);
        }

        shader.color = {std::rand() % 255, std::rand() % 255, std::rand() % 255, 255};

        rasterize(pipeline, clip, shader, framebuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");

    // depth buffer TGA image for debugging
    TGAImage depthimg(width, height, TGAImage::GRAYSCALE);

    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();

    for (float depth : pipeline.depthbuffer) {
        if (depth == -std::numeric_limits<float>::infinity()) continue;

        if (depth > zmax) zmax = depth;
        if (depth < zmin) zmin = depth;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width;
            float z = pipeline.depthbuffer[idx];

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
