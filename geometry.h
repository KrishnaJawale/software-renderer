#pragma once
#include <cmath>

struct Vec3f {
    float x, y, z;

    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}

    float operator*(const Vec3f &v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vec3f operator+(const Vec3f &v) const {
        return Vec3f(x + v.x, y + v.y, z + v.z);
    }

    Vec3f operator-(const Vec3f &v) const {
        return Vec3f(x - v.x, y - v.y, z - v.z);
    }

    Vec3f operator*(float s) const {
        return Vec3f(x * s, y * s, z * s);
    }

    Vec3f operator/(float s) const {
        return Vec3f(x / s, y / s, z / s);
    }
};

struct Vec3i {
    int x, y, z;

    Vec3i() : x(0), y(0), z(0) {}
    Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
};

struct Mat3f {
    Vec3f rows[3];

    Vec3f operator*(const Vec3f &v) const {
        return Vec3f(
            rows[0] * v,
            rows[1] * v,
            rows[2] * v
        );
    }
};
