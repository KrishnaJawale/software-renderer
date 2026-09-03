#pragma once
#include "tgaimage.h"
#include "geometry.h"
#include <utility>
#include <vector>

struct Pipeline {
    Mat4f ModelView, Viewport, Projection;
    std::vector<float> depthbuffer;

    void lookat(const Vec3f &eye, const Vec3f &center, const Vec3f &up);
    void set_viewport(const int x, const int y, const int w, const int h);
    void set_perspective(const double focal_length);
    void init_depthbuffer(const int width, const int height);
};

struct IShader {
    virtual ~IShader() = default;
    virtual std::pair<bool, TGAColor> fragment(const Vec3f &bar) const = 0;
};

void rasterize(Pipeline &pipeline, const Vec4f clip[3], const IShader &shader, TGAImage &framebuffer);
