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

struct PhongShader : IShader {    
    Vec3f normal{}; // unit vector orthogonal to the triangle
    Vec3f light_dir = normalize(Vec3f{1.0f, 1.0f, 1.0f}); // direction of the light source
    Vec3f view_dir = normalize(Vec3f{0.f, 0.f, -1.f}); // direction of the view vector
    float shininess = 100.f;

    std::pair<bool, TGAColor> fragment(const Vec3f &bar) const override {
        float ambient = 0.1f;
        float diffuse = std::max(0.f, dot(normal, light_dir));
        Vec3f light_reflection = normal * 2 * dot(normal, light_dir) - light_dir;
        light_reflection = normalize(light_reflection);
        float specular = std::pow(std::max(0.f, dot(light_reflection, view_dir)), shininess);
        float intensity = std::min(1.0f, ambient + diffuse + specular) * 255.f;
        std::uint8_t intensity_u8 = static_cast<std::uint8_t>(intensity);

        TGAColor color{intensity_u8, intensity_u8, intensity_u8, 255};

        return {false, color};
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
    Model model("models/head.obj");

    PhongShader shader;

    // draw faces
    for (const auto& face : model.faces) {
        Vec3f verts[3] = {
            model.verts[face.x],
            model.verts[face.y],
            model.verts[face.z]
        };

        Vec3f view_pos[3];
        Vec4f clip[3];
        for (int i = 0; i < 3; i++) {
            Vec4f view = pipeline.ModelView * Vec4f(verts[i].x, verts[i].y, verts[i].z, 1.f);
            clip[i] = pipeline.Projection * view;

            view_pos[i] = Vec3f(view.x, view.y, view.z);
        }

        // shader.color = {std::rand() % 255, std::rand() % 255, std::rand() % 255, 255};
        // compute normal to the triangle face for Phong shader
        Vec3f normal = cross(view_pos[1] - view_pos[0], view_pos[2] - view_pos[0]);
        normal = normalize(normal);
        shader.normal = normal;

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
