#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

struct Vec3 {
    double x, y, z;
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double v) : x(v), y(v), z(v) {}
    Vec3(Vec3 const& other) : x(other.x), y(other.y), z(other.z) {}

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

    // Multiplication composante-à-composante (utile pour couleurs)
    Vec3 operator*(const Vec3& other) const
    {
        return { x * other.x, y * other.y, z * other.z };
    }

    // opérateur += correct (modifie l'objet)
    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3 operator/(double s) const 
    { 
        return { x / s, y / s, z / s }; 
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

    double length() const
    {
        return std::sqrt(this->dot(*this));
    }

    Vec3 normalize() const
    {
        double len = length();
        if (len == 0.0) return Vec3(0,0,0);
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
        double sx = (pixelX + 0.5) - width / 2.0;
        double sy = height / 2.0 - (pixelY + 0.5);
        Vec3 screenPoint = Vec3(sx, sy, position.z + focal);
        Vec3 direction = (screenPoint - position).normalize();
        return Rayon(position, direction);
    }
};