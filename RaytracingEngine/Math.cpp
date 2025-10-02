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

    Vec3 operator*(const float amount) const
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
};

struct Sphere {
    Vec3 center, color;
    double radius;
	Sphere(const Vec3& center, double radius, const Vec3& color = Vec3(1, 1, 1)) : center(center), radius(radius), color(color) {}
	Sphere() : center(Vec3(0, 0, 0)), radius(1), color(Vec3(1, 1, 1)) {}

    std::optional<double> intersect(const Rayon& ray) const
    {
        Vec3 oc = ray.origin - center;

        double a = ray.direction.dot(ray.direction);
        double b = 2.0 * oc.dot(ray.direction);
        double c = oc.dot(oc) - radius * radius;

        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) {
            return std::nullopt;
        } else {
            return { ((-b - std::sqrt(discriminant)) / (2.0 * a)) };
		}
	}
};

struct Camera {
    Vec3 position;
	float distanceToScreen;
	float width, height;

	Camera(Vec3& position, float distanceToScreen, float width, float height) : position(position), distanceToScreen(distanceToScreen), width(width), height(height) {}

    Rayon getRay(float x, float y) const
    {
        Vec3 screenPoint = Vec3(x, y, position.z + distanceToScreen);
        Vec3 direction = (screenPoint - position).normalize();
        return Rayon(position, direction);
    }
};