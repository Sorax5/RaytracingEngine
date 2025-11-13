#pragma once

#include <vector>
#include <optional>
#include "Math.h"

struct Transform {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

struct Material {
    Vec3 color;
    double shininess = 128.0;
    double specular = 0.0;
    double transparency = 0.0;
    double refractiveIndex = 1.0;
};

enum class HitType: unsigned char {
    NONE,
    SPHERE,
    PLANE
};

struct HitInfo {
    double distance;
    HitType type;
    size_t index;
    Material material;
    Vec3 normal;
    Vec3 hitPoint;

    bool isCloserThan(const HitInfo& other) const {
        return distance < other.distance;
    }

    double normalizedDistance(const Camera& camera) const {
        return (distance - camera.nearPlaneDistance) / (camera.farPlaneDistance - camera.nearPlaneDistance);
    }

    static HitInfo getClosestIntersection(const std::vector<HitInfo>& intersections) {
        HitInfo closestIntersection = intersections[0];
        for (auto info : intersections)
        {
            if (info.isCloserThan(closestIntersection))
            {
                closestIntersection = info;
            }
        }

        return closestIntersection;
    }

};

class Sphere {
private:
    double radius;
    Transform transform;
    Material material;
public:
    Sphere(const double r = 1.0, const Vec3& pos = Vec3(0, 0, 0), const Material& mat = Material()) : radius(r) {
        transform.position = pos;
        transform.rotation = Vec3(0, 0, 0);
        transform.scale = Vec3(1, 1, 1);
        this->material = mat;
    }

    std::optional<double> intersect(const Rayon& ray) const {
        const Vec3 oc = ray.origin - transform.position;

        const double a = ray.direction.dot(ray.direction);
        const double b = 2.0 * oc.dot(ray.direction);
        const double c = oc.dot(oc) - radius * radius;

        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) {
            return std::nullopt;
        }

        const double sqrtDisc = std::sqrt(discriminant);
        double t0 = (-b - sqrtDisc) / (2.0 * a);
        double t1 = (-b + sqrtDisc) / (2.0 * a);
        if (t0 > t1) std::swap(t0, t1);

        const double eps = 1e-6;
        double t = t0;
        if (t < eps) {
            t = t1;
            if (t < eps) {
                return std::nullopt;
            }
        }
        return t;
    }

    std::optional<Vec3> getNormalAt(const Vec3& point) const {
        return (point - transform.position).normalize();
    }

    std::optional<HitInfo> getHitInfoAt(const Rayon& ray, const size_t index) const {
        if (const auto intersectionOpt = intersect(ray); intersectionOpt) {
            const double intersection = intersectionOpt.value();

            const Vec3 hitPoint = ray.pointAtDistance(intersection);
            const Vec3 normal = getNormalAt(hitPoint).value();
            return HitInfo{
                intersection,
                HitType::SPHERE,
                index,
                material,
                normal,
                hitPoint
            };
        }

        return std::nullopt;
    }

    double getRadius() const { return radius; }
    void setRadius(double r) { radius = r; }

    Material getMaterial() const { return material; }
    Transform getTransform() const { return transform; }

    static Sphere getHitObject(const HitInfo& hit, const std::vector<Sphere>& spheres)
    {
        return spheres[hit.index];
    }
};

class Plane {
private:
    Vec3 normal;
    Transform transform;
    Material material;
public:
    Plane(const Vec3& pos = Vec3(0, 1, 0), const Vec3& norm = Vec3(0, 1, 0), const Material& material = Material())
        : normal(norm.normalize()) {
        transform.position = pos;
        transform.rotation = Vec3(0, 0, 0);
        transform.scale = Vec3(1, 1, 1);
        this->material = material;
    }

    std::optional<double> intersect(const Rayon& ray) const {
        const double denom = normal.dot(ray.direction);
        if (std::abs(denom) > 1e-6) {
            const Vec3 p0l0 = transform.position - ray.origin;
            const double t = p0l0.dot(normal) / denom;
            if (t >= 0.0) {
                return { t };
            }
        }
        return std::nullopt;
    }

    std::optional<Vec3> getNormalAt() const {
        return normal;
    }

    std::optional<HitInfo> getHitInfoAt(const Rayon& ray, const size_t index) const {
        if (const auto intersectionOpt = intersect(ray); intersectionOpt)
        {
            const double intersection = intersectionOpt.value();
            const Vec3 hitPoint = ray.pointAtDistance(intersection);
            const Vec3 normal = getNormalAt().value();
            return HitInfo{
                intersection,
                HitType::PLANE,
                index,
                material,
                normal,
                hitPoint
            };
        }

        return std::nullopt;
    }

    Vec3 getNormal() const { return normal; }
    void setNormal(const Vec3& norm) { normal = norm.normalize(); }
    Material getMaterial() const { return material; }
    Transform getTransform() const { return transform; }

    static Plane getHitObject(const HitInfo& hit, const std::vector<Plane>& planes)
    {
        return planes[hit.index];
    }
};