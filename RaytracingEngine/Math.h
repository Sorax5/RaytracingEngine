#pragma once

#include <cstdint>
#include <stdexcept>
#include <random>
#include <algorithm>

struct Vec3 {
    double x, y, z;
    Vec3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}
    Vec3(double v) : x(v), y(v), z(v) {}

    Vec3 operator+(const Vec3& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
	Vec3 operator+(const double s) const noexcept { return { x + s, y + s, z + s }; }
    Vec3 operator-(const Vec3& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
	Vec3 operator-(const double s) const noexcept { return { x - s, y - s, z - s }; }
    Vec3 operator*(const double s) const noexcept { return { x * s, y * s, z * s }; }
    Vec3 operator*(const Vec3& o) const noexcept { return { x * o.x, y * o.y, z * o.z }; }
    Vec3 operator/(const Vec3& o) const noexcept { return { x / o.x, y / o.y, z / o.z }; }
    Vec3 operator/(const double s) const noexcept { return { x / s, y / s, z / s }; }
    Vec3& operator+=(const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3 operator-() const noexcept { return { -x, -y, -z }; }
    Vec3& operator/=(const double s) noexcept { x /= s; y /= s; z /= s; return *this; }
	Vec3& operator*=(const double s) noexcept { x *= s; y *= s; z *= s; return *this; }

    double dot(const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const noexcept { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }

    double length() const noexcept { return std::sqrt(dot(*this)); }
    Vec3 normalize() const noexcept {
        double len = length();
		if (len <= 1e-12)
		{
            return Vec3{ 0, 0, 0 };
		}
        return (*this) / len;
    }

    Vec3 reflect(const Vec3 normal) const
    {
        return (*this) - normal * 2.0 * this->dot(normal);
    }

    Vec3 refract(const Vec3& normal, double eta) const {
        Vec3 I = this->normalize();
        Vec3 N = normal.normalize();
        double cosi = std::clamp(I.dot(N), -1.0, 1.0);
        double k = 1.0 - eta * eta * (1.0 - cosi * cosi);
        if (k < 0.0) {
            return Vec3{ 0,0,0 };
        }
        return I * eta - N * (eta * cosi + std::sqrt(k));
    }

    double unsafeIndex(const int index) const {
        switch (index) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: throw std::out_of_range("Vec3 index out of range");
        }
    }

    Vec3& lerp(const Vec3& target, const double t) noexcept {
        x = x + (target.x - x) * t;
        y = y + (target.y - y) * t;
        z = z + (target.z - z) * t;
        return *this;
	}

    friend Vec3 operator*(double s, const Vec3& v) noexcept { return v * s; }
};

struct Color {
    uint8_t r, g, b;
    Color(const uint8_t r = 0, const uint8_t g = 0, const uint8_t b = 0) : r(r), g(g), b(b) {}
};

struct Rayon {
    Vec3 origin;
    Vec3 direction;
    Rayon(const Vec3& o = {}, const Vec3& d = {}) : origin(o), direction(d) {}
    Vec3 pointAtDistance(const double t) const noexcept { return origin + direction * t; }
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

    Rayon getRay(const size_t pixelX, const size_t pixelY, const bool aa) const {
        auto sx = (static_cast<double>(pixelX) ) - static_cast<double>(width) / 2.0;
        auto sy = static_cast<double>(height) / 2.0 - (static_cast<double>(pixelY));

		auto jitterX = 0.0;
		auto jitterY = 0.0;
		if (aa) {
			const auto invAA = 1.0 / static_cast<double>(aa);

			// suggestion Chatgpt : use thread_local random generators to avoid contention in multithreaded scenarios
			thread_local static std::mt19937 gen((std::random_device())());
			thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);
			jitterX = dist(gen) * invAA;
			jitterY = dist(gen) * invAA;
		}

		sx += jitterX;
		sy += jitterY;

        const auto screenPoint = Vec3(sx, sy, position.z + focal);
        const auto dir = (screenPoint - position).normalize();
        return Rayon{ position, dir };
    }
};

