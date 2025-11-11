#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <cstddef>
#include <iostream>
#include <random>

struct Vec3 {
    double x, y, z;
    Vec3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}
    Vec3(double v) : x(v), y(v), z(v) {}

    Vec3 operator+(const Vec3& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
	Vec3 operator+(const double s) const noexcept { return { x + s, y + s, z + s }; }
    Vec3 operator-(const Vec3& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
	Vec3 operator-(double s) const noexcept { return { x - s, y - s, z - s }; }
    Vec3 operator*(double s) const noexcept { return { x * s, y * s, z * s }; }
    Vec3 operator*(const Vec3& o) const noexcept { return { x * o.x, y * o.y, z * o.z }; }
    Vec3 operator/(const Vec3& o) const noexcept { return { x / o.x, y / o.y, z / o.z }; }
    Vec3 operator/(double s) const noexcept { return { x / s, y / s, z / s }; }
    Vec3& operator+=(const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3 operator-() const noexcept { return { -x, -y, -z }; }
    Vec3& operator/=(double s) noexcept { x /= s; y /= s; z /= s; return *this; }
	Vec3& operator*=(double s) noexcept { x *= s; y *= s; z *= s; return *this; }

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

    Vec3& lerp(const Vec3& target, double t) noexcept {
        x = x + (target.x - x) * t;
        y = y + (target.y - y) * t;
        z = z + (target.z - z) * t;
        return *this;
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

	int antiAliasingAmount = 32;

    Camera(const Vec3& position = {0,0,0}, double focal = 1.0, std::size_t width = 800, std::size_t height = 600, double nearPlaneDistance = 1.0, double farPlaneDistance = 1000.0)
        : position(position), forward{0,0,1}, width(width), height(height), focal(focal), farPlaneDistance(farPlaneDistance), nearPlaneDistance(nearPlaneDistance) {}

    Rayon getRay(std::size_t pixelX, std::size_t pixelY, bool aa) const {
        double sx = (static_cast<double>(pixelX) ) - static_cast<double>(width) / 2.0;
        double sy = static_cast<double>(height) / 2.0 - (static_cast<double>(pixelY));

		double jitterX = 0.0;
		double jitterY = 0.0;
		if (aa) {
			double invAA = 1.0 / static_cast<double>(aa);
			// RNG thread-local pour éviter contention et rendre reproductible en multithread
			thread_local static std::mt19937 gen((std::random_device())());
			thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);
			jitterX = dist(gen) * invAA;
			jitterY = dist(gen) * invAA;
		}

		sx += jitterX;
		sy += jitterY;

        Vec3 screenPoint = Vec3(sx, sy, position.z + focal);
        Vec3 dir = (screenPoint - position).normalize();
        return Rayon(position, dir);
    }
};

