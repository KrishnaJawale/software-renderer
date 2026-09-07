#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "pipeline.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// Decode an object space normal from a normal map texel
// TGAColor is BGRA, but the map stores XYZ in RGB channels
static Vec3f normal_from_map(const TGAImage &normal_map, const Vec2f &tex_coord) {
    const int x = std::clamp(static_cast<int>(tex_coord.x * normal_map.width()), 0, normal_map.width() - 1);
    const int y = std::clamp(static_cast<int>(tex_coord.y * normal_map.height()), 0, normal_map.height() - 1);
    const TGAColor texel = normal_map.get(x, y);
    return Vec3f(
        texel[2] / 255.f * 2.f - 1.f, // get x component from red channel
        texel[1] / 255.f * 2.f - 1.f, // get y component from green channel
        texel[0] / 255.f * 2.f - 1.f  // get z component from blue channel
    );
}

static bool face_has_valid_uvs(const Model &model, const Vec3i &face_tex) {
    if (model.tex_coords.empty()) {
        return false;
    }
    const int n = static_cast<int>(model.tex_coords.size());
    return face_tex.x >= 0 && face_tex.x < n
        && face_tex.y >= 0 && face_tex.y < n
        && face_tex.z >= 0 && face_tex.z < n;
}

struct PhongShader : IShader {
    Vec2f triangle_uvs[3]{};
    Vec3f flat_normal{}; // view space face normal (fallback)
    bool use_normal_map = false;
    const TGAImage *normal_map = nullptr;
    Mat4f ModelView{}; // need copy of camera matrix to map normals to view space

    Vec3f light_dir = normalize(Vec3f{1.0f, 1.0f, 1.0f});
    Vec3f view_dir = normalize(Vec3f{0.f, 0.f, -1.f});
    float shininess = 100.f;

    std::pair<bool, TGAColor> fragment(const Vec3f &barycentric) const override {
        Vec3f normal;
        if (use_normal_map && normal_map) {
            Vec2f tex_coord =
                triangle_uvs[0] * barycentric.x +
                triangle_uvs[1] * barycentric.y +
                triangle_uvs[2] * barycentric.z;
            Vec3f object_normal = normal_from_map(*normal_map, tex_coord);
            Vec4f view_normal4 = ModelView * Vec4f(object_normal.x, object_normal.y, object_normal.z, 0.f);
            normal = normalize(Vec3f(view_normal4.x, view_normal4.y, view_normal4.z));
        } else {
            normal = flat_normal;
        }

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

static void render_frame(Pipeline &pipeline, TGAImage &framebuffer, Model &model, PhongShader &shader,
                         const Vec3f &eye, const Vec3f &center, const Vec3f &up) {
    const int width = framebuffer.width();
    const int height = framebuffer.height();

    pipeline.set_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    pipeline.set_perspective(norm(eye - center));
    pipeline.lookat(eye, center, up);
    pipeline.init_depthbuffer(width, height);
    shader.ModelView = pipeline.ModelView;

    // clear framebuffer to black
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer.set(x, y, {0, 0, 0, 255});
        }
    }

    for (int face_index = 0; face_index < model.faces.size(); face_index++) {
        const Vec3i &face = model.faces[face_index];
        const Vec3i &face_tex = model.face_tex_coords[face_index];

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

        // Always compute a flat face normal, used when UVs or normal map are unavailable as fallback
        shader.flat_normal = normalize(cross(view_pos[1] - view_pos[0], view_pos[2] - view_pos[0]));

        const bool can_sample_map = shader.normal_map && face_has_valid_uvs(model, face_tex);
        shader.use_normal_map = can_sample_map;
        if (can_sample_map) {
            shader.triangle_uvs[0] = model.tex_coords[face_tex.x];
            shader.triangle_uvs[1] = model.tex_coords[face_tex.y];
            shader.triangle_uvs[2] = model.tex_coords[face_tex.z];
        }

        rasterize(pipeline, clip, shader, framebuffer);
    }
}

int main(int argc, char **argv) {
    constexpr int width = 1024;
    constexpr int height = 1024;
    const Vec3f eye{-1.f, 0.f, 4.0f};
    const Vec3f center{0.f, 0.f, 0.f};
    const Vec3f up{0.f, 1.f, 0.f};

    const std::string model_path = (argc > 1) ? argv[1] : "models/totem.obj";
    const std::string normal_map_path = (argc > 2) ? argv[2] : "models/african_head_nm.tga";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "software-renderer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        0);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // TGA RGB framebuffer is stored as B,G,R per pixel
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_BGR24,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Pipeline pipeline;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    Model model(model_path);
    if (model.faces.empty()) {
        std::cerr << "failed to load model (no faces): " << model_path << "\n";
        return 1;
    }

    PhongShader shader;
    TGAImage normal_map;
    if (normal_map.read_tga_file(normal_map_path)) {
        normal_map.flip_vertically();
        shader.normal_map = &normal_map;
        std::cerr << "normal map loaded: " << normal_map_path << "\n";
    } else {
        shader.normal_map = nullptr;
        std::cerr << "no normal map (using flat shading fallback)\n";
    }

    if (model.tex_coords.empty()) {
        std::cerr << "model has no UVs — normal map sampling disabled\n";
    }

    render_frame(pipeline, framebuffer, model, shader, eye, center, up);
    framebuffer.write_tga_file("framebuffer.tga", false); // false = top left origin (matches SDL buffer)

    SDL_UpdateTexture(texture, nullptr, framebuffer.buffer(), width * framebuffer.bytes_per_pixel());

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 Hz present; scene is static
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
