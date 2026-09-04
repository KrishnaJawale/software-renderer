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
#include <vector>

struct PhongShader : IShader {
    Vec3f normal{}; // unit vector orthogonal to the triangle
    Vec3f light_dir = normalize(Vec3f{1.0f, 1.0f, 1.0f}); // from surface toward the light
    Vec3f view_dir = normalize(Vec3f{0.f, 0.f, -1.f}); // from surface toward the camera
    float shininess = 100.f;

    std::pair<bool, TGAColor> fragment(const Vec3f &) const override {
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

    // clear framebuffer to black
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer.set(x, y, {0, 0, 0, 255});
        }
    }

    for (const auto &face : model.faces) {
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

        shader.normal = normalize(cross(view_pos[1] - view_pos[0], view_pos[2] - view_pos[0]));
        rasterize(pipeline, clip, shader, framebuffer);
    }
}

int main(int argc, char **argv) {
    constexpr int width = 1024;
    constexpr int height = 1024;
    const Vec3f eye{-1.f, 0.f, 2.f};
    const Vec3f center{0.f, 0.f, 0.f};
    const Vec3f up{0.f, 1.f, 0.f};

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
    Model model("models/beast.obj");
    PhongShader shader;

    // Present-only: render once, then keep showing it
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
