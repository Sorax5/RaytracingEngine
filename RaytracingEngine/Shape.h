#pragma once

#include <vector>
#include <optional>
#include <variant>
#include "Math.cpp"

template <typename Derived>
class Shape {
private:
    Vec3 position;
    Vec3 color;
public:
    Shape(const Vec3& pos = Vec3(0, 0, 0), const Vec3& col = Vec3(1, 1, 1))
        : position(pos), color(col) {
    }

    std::optional<double> intersect(const Rayon& ray) const {
        return static_cast<const Derived&>(*this).intersect_impl(ray);
    }

    std::optional<Vec3> getNormalAt(const Vec3& point) const {
        return static_cast<const Derived&>(*this).getNormalAt_impl(point);
    }

    Vec3 getPosition() const { return position; }
    Vec3 getColor() const { return color; }
    void setPosition(const Vec3& pos) { position = pos; }
    void setColor(const Vec3& col) { color = col; }
};

class Sphere : public Shape<Sphere> {
private:
    double radius;
public:
    Sphere(const Vec3& pos = Vec3(0, 0, 0),
        double r = 1,
        const Vec3& col = Vec3(1, 1, 1))
        : Shape<Sphere>(pos, col), radius(r) {}

    std::optional<double> intersect_impl(const Rayon& ray) const {
        Vec3 oc = ray.origin - getPosition();
        double a = ray.direction.dot(ray.direction);
        double b = 2.0 * oc.dot(ray.direction);
        double c = oc.dot(oc) - radius * radius;
        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) {
            return std::nullopt;
        }
        else {
            return { ((-b - std::sqrt(discriminant)) / (2.0 * a)) };
        }
    }
    std::optional<Vec3> getNormalAt_impl(const Vec3& point) const {
        Vec3 normal = (point - this->getPosition()).normalize();
        return normal;
    }

    double getRadius() const { return radius; }
    void setRadius(double r) { radius = r; }
};

class Plane : public Shape<Plane> {
private:
    Vec3 normal;
public:
    Plane(const Vec3& pos = Vec3(0, 0, 0),
        const Vec3& norm = Vec3(0, 1, 0),
        const Vec3& col = Vec3(1, 1, 1))
        : Shape<Plane>(pos, col), normal(norm) {}

    std::optional<double> intersect_impl(const Rayon& ray) const {
        double denom = normal.dot(ray.direction);
        if (std::abs(denom) > 1e-6) {
            Vec3 p0l0 = getPosition() - ray.origin;
            double t = p0l0.dot(normal) / denom;
            if (t >= 0) {
                return { t };
            }
        }
        return std::nullopt;
    }
    std::optional<Vec3> getNormalAt_impl(const Vec3& /*point*/) const {
        return normal;
    }

    Vec3 getNormal() const { return normal; }
    void setNormal(const Vec3& norm) { normal = norm; }
};

using ShapeVariant = std::variant<Sphere, Plane>;

inline std::optional<double> intersect(const ShapeVariant& shape, const Rayon& ray) {
    return std::visit([&](auto const& s) { return s.intersect(ray); }, shape);
}

inline std::optional<Vec3> getNormalAt(const ShapeVariant& shape, const Vec3& point) {
    return std::visit([&](auto const& s) { return s.getNormalAt(point); }, shape);
}

inline Vec3 getColor(const ShapeVariant& shape) {
    return std::visit([&](auto const& s) { return s.getColor(); }, shape);
}
