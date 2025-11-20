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
	PLANE,
	TRIANGLE
};

struct HitInfo {
    HitType type;
    double distance;
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
    explicit Sphere(const double r = 1.0, const Vec3& pos = Vec3(0, 0, 0), const Material& mat = Material()) : radius(r) {
        transform.position = pos;
        transform.rotation = Vec3(0, 0, 0);
        transform.scale = Vec3(1, 1, 1);
        this->material = mat;
    }

    std::optional<double> Intersect(const Rayon& ray) const {
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

    std::optional<Vec3> GetNormalAt(const Vec3& point) const {
        return (point - transform.position).normalize();
    }

    std::optional<HitInfo> GetHitInfoAt(const Rayon& ray, const size_t index) const {
        if (const auto intersectionOpt = Intersect(ray); intersectionOpt) {
            const double intersection = intersectionOpt.value();

            const Vec3 hitPoint = ray.pointAtDistance(intersection);
            const Vec3 normal = GetNormalAt(hitPoint).value();
            return HitInfo{
				.type = HitType::SPHERE,
				.distance = intersection,
				.index = index,
				.material = material,
				.normal = normal,
				.hitPoint = hitPoint
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
            return HitInfo {
				.type = HitType::PLANE,
				.distance = intersection,
				.index = index,
				.material = material,
				.normal = getNormalAt().value(),
				.hitPoint = ray.pointAtDistance(intersection)
            };
        }

        return std::nullopt;
    }

	Vec3 getNormal() const { return normal; }
	void setNormal(const Vec3& norm) { normal = norm.normalize(); }
	Material getMaterial() const { return material; }
	Transform getTransform() const { return transform; }
};

class Triangle {
private:
	Vec3 v0, v1, v2;
	Material material;
public:
	Triangle(const Vec3& vertex0, const Vec3& vertex1, const Vec3& vertex2, const Vec3& col = Vec3(1, 1, 1))
		: v0(vertex0), v1(vertex1), v2(vertex2) {
		material.color = col;
	}

	std::optional<double> intersect(const Rayon& ray) const {
		const double EPSILON = 1e-6;
		Vec3 edge1 = v1 - v0;
		Vec3 edge2 = v2 - v0;
		Vec3 h = ray.direction.cross(edge2);
		double a = edge1.dot(h);
		if (a > -EPSILON && a < EPSILON)
			return std::nullopt;
		double f = 1.0 / a;
		Vec3 s = ray.origin - v0;
		double u = f * s.dot(h);
		if (u < 0.0 || u > 1.0)
			return std::nullopt;
		Vec3 q = s.cross(edge1);
		double v = f * ray.direction.dot(q);
		if (v < 0.0 || u + v > 1.0)
			return std::nullopt;
		double t = f * edge2.dot(q);
		if (t > EPSILON)
			return { t };
		else
			return std::nullopt;
	}

	std::optional<Vec3> getNormalAt() const {
		Vec3 normal = (v1 - v0).cross(v2 - v0).normalize();
		return normal;
	}

	std::optional<HitInfo> getHitInfoAt(const Rayon& ray, size_t index) const {
		std::optional<double> intersection = intersect(ray);
		if (intersection.has_value())
		{
			Vec3 hitPoint = ray.pointAtDistance(intersection.value());
			Vec3 normal = getNormalAt().value();
			return HitInfo{
				intersection.value(),
				HitType::TRIANGLE,
				index,
				material,
				normal,
				hitPoint
			};
		}
		return std::nullopt;
	}

	Material getMaterial() const { return material; }
};