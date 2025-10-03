#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

struct Vec3 {
    double x, y, z;
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
	Vec3() : x(0), y(0), z(0) {}

    Vec3 operator+(const Vec3& other) const
    {
        return { x + other.x, y + other.y, z + other.z };
    }

    Vec3 operator-(const Vec3& other) const
    {
        return { x - other.x, y - other.y, z - other.z };
    }

    Vec3 operator*(const double amount) const
    {
        return { x * amount, y * amount, z * amount };
    }

    Vec3 operator*(const Vec3& other) const
    {
        return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x };
    }

    Vec3 operator+=(const Vec3& other) const
    {
		return { x + other.x, y + other.y, z + other.z };
	}

    double unsafeIndex(int index)
    {
        switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw "Out of bounds";
        }
    }

    double dot(const Vec3& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    double length()
    {
        return std::sqrt(this->dot(*this));
    }

    Vec3 normalize()
    {
        double len = length();
        return { x / len, y / len, z / len };
	}
};

struct Color {
	uint8_t r, g, b;
	Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
	Color() : r(0), g(0), b(0) {}
};

struct Rayon {
    Vec3 origin;
    Vec3 direction;
    Rayon(const Vec3& origin, const Vec3& direction) : origin(origin), direction(direction) {}
    Vec3 pointAtDistance(double t) const
    {
        return origin + direction * t;
	}
};

struct Camera {
    Vec3 position;
	Vec3 forward;

	double width, height;
    double focal;
    double farPlaneDistance;
    double nearPlaneDistance;

    Camera(const Vec3& position, double focal, double width, double height, double nearPlaneDistance = 1.0f, double farPlaneDistance = 1000.0f)
        : position(position), focal(focal), width(width), height(height), nearPlaneDistance(nearPlaneDistance), farPlaneDistance(farPlaneDistance)
    {
		forward = Vec3(0, 0, 1);
	}

    Rayon getRay(int pixelX, int pixelY) const
    {
		Vec3 screenPoint = Vec3((pixelX - width / 2.0f), (pixelY - height / 2.0f), position.z + focal);
        Vec3 direction = (screenPoint - position).normalize();
		return Rayon(position, direction);
	}
};