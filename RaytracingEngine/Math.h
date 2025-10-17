#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <cstddef>

struct Vec3 {
    double x, y, z;
    Vec3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}
    Vec3(double v) : x(v), y(v), z(v) {}

    Vec3 operator+(const Vec3& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(double s) const noexcept { return { x * s, y * s, z * s }; }
    Vec3 operator*(const Vec3& o) const noexcept { return { x * o.x, y * o.y, z * o.z }; }
    Vec3 operator/(double s) const noexcept { return { x / s, y / s, z / s }; }
    Vec3& operator+=(const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3 operator-() const noexcept { return { -x, -y, -z }; }

    double dot(const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const noexcept { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }

    double length() const noexcept { return std::sqrt(dot(*this)); }
    Vec3 normalize() const noexcept {
        double len = length();
        if (len <= 1e-12) return Vec3(0.0, 0.0, 0.0);
        return (*this) / len;
    }

    double unsafeIndex(int index) const {
        switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw std::out_of_range("Vec3 index out of range");
        }
    }

    friend Vec3 operator*(double s, const Vec3& v) noexcept { return v * s; }
};

struct Color {
    uint8_t r, g, b;
    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : r(r), g(g), b(b) {}
};

struct Rayon {
    Vec3 origin;
    Vec3 direction;
    Rayon(const Vec3& o = {}, const Vec3& d = {}) : origin(o), direction(d) {}
    Vec3 pointAtDistance(double t) const noexcept { return origin + direction * t; }
};

struct Camera {
    Vec3 position;
    Vec3 forward;
    std::size_t width;
    std::size_t height;
    double focal;
    double farPlaneDistance;
    double nearPlaneDistance;

    Camera(const Vec3& position = {0,0,0}, double focal = 1.0, std::size_t width = 800, std::size_t height = 600, double nearPlaneDistance = 1.0, double farPlaneDistance = 1000.0)
        : position(position), forward{0,0,1}, width(width), height(height), focal(focal), farPlaneDistance(farPlaneDistance), nearPlaneDistance(nearPlaneDistance) {}

    Rayon getRay(std::size_t pixelX, std::size_t pixelY) const {
        double sx = (static_cast<double>(pixelX) + 0.5) - static_cast<double>(width) / 2.0;
        double sy = static_cast<double>(height) / 2.0 - (static_cast<double>(pixelY) + 0.5);
        Vec3 screenPoint = Vec3(sx, sy, position.z + focal);
        Vec3 dir = (screenPoint - position).normalize();
        return Rayon(position, dir);
    }
};