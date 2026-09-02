#pragma once
#include <cmath>

struct Vec3f {
    float x, y, z;

    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}

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

    float operator[](int i) const {
        return i == 0 ? x : i == 1 ? y : z;
    }

    const float& operator[](int i) const {
        return i == 0 ? x : i == 1 ? y : z;
    }
};

struct Vec4f {
    float x, y, z, w;

    Vec4f() : x(0), y(0), z(0), w(0) {}
    Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    float operator*(const Vec4f &v) const {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    Vec4f operator+(const Vec4f &v) const {
        return Vec4f(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    Vec4f operator-(const Vec4f &v) const {
        return Vec4f(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    Vec4f operator*(float s) const {
        return Vec4f(x * s, y * s, z * s, w * s);
    }

    Vec4f operator/(float s) const {
        return Vec4f(x / s, y / s, z / s, w / s);
    }

    float& operator[](int i) {
        return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
    }
    const float& operator[](int i) const {
        return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
    }
};

struct Vec3i {
    int x, y, z;

    Vec3i() : x(0), y(0), z(0) {}
    Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}

    int operator[](int i) const {
        return i == 0 ? x : i == 1 ? y : z;
    }
};

float dot(const Vec3f &v1, const Vec3f &v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float dot(const Vec4f &v1, const Vec4f &v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

float length(const Vec3f &v) {
    return std::sqrt(dot(v, v));
}

Vec3f normalize(const Vec3f &v) {
    return v / length(v);
}

Vec3f cross(const Vec3f &v1, const Vec3f &v2) {
    return Vec3f(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    );
}

struct Mat3f {
    Vec3f rows[3];

    Vec3f operator*(const Vec3f &v) const {
        return Vec3f(
            dot(rows[0], v),
            dot(rows[1], v),
            dot(rows[2], v)
        );
    }
};

struct Mat4f {
    Vec4f rows[4];

    Vec4f operator*(const Vec4f &v) const {
        return Vec4f(
            dot(rows[0], v),
            dot(rows[1], v),
            dot(rows[2], v),
            dot(rows[3], v)
        );
    }

    Mat4f operator*(const Mat4f &m) const {
        Mat4f result;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                result.rows[r][c] = 0;
                for (int i = 0; i < 4; i++) {
                    result.rows[r][c] += rows[r][i] * m.rows[i][c];
                }
            }
        }

        return result;
    }
};
