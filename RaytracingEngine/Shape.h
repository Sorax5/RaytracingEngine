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

    std::optional<double> Intersect(const Rayon& ray) const {
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

    Vec3 GetNormalAt() const {
        return normal;
    }

    std::optional<HitInfo> GetHitInfoAt(const Rayon& ray, const size_t index) const {
        if (const auto intersectionOpt = Intersect(ray); intersectionOpt)
        {
            const double intersection = intersectionOpt.value();
            return HitInfo {
				.type = HitType::PLANE,
				.distance = intersection,
				.index = index,
				.material = material,
				.normal = GetNormalAt(),
				.hitPoint = ray.pointAtDistance(intersection)
            };
        }

        return std::nullopt;
    }

	Vec3 GetNormal() const { return normal; }
	void SetNormal(const Vec3& norm) { normal = norm.normalize(); }
	Material GetMaterial() const { return material; }
	Transform GetTransform() const { return transform; }
};

class Triangle {
private:
	Vec3 v0, v1, v2;
	Transform transform; // now stored
	Material material;
public:
	Triangle(const Vec3& vertex0, const Vec3& vertex1, const Vec3& vertex2, const Material& mat = Material(), const Transform& t = Transform())
		: v0(vertex0), v1(vertex1), v2(vertex2), transform(t), material(mat) {}

	// Apply translation (and simple uniform scale) from transform for intersection tests
	Vec3 tv0() const { return v0 + transform.position; }
	Vec3 tv1() const { return v1 + transform.position; }
	Vec3 tv2() const { return v2 + transform.position; }

	std::optional<double> Intersect(const Rayon& ray) const {
		constexpr double EPSILON = 1e-6;
		const auto a0 = tv0();
		const auto edge1 = tv1() - a0;
		const auto edge2 = tv2() - a0;

		const auto h = ray.direction.cross(edge2);
		const double a = edge1.dot(h);
		if (a > -EPSILON && a < EPSILON) { return std::nullopt; }
		const double f = 1.0 / a;
		const auto s = ray.origin - a0;
		const double u = f * s.dot(h);
		if (u < 0.0 || u > 1.0) { return std::nullopt; }
		const Vec3 q = s.cross(edge1);
		const double v = f * ray.direction.dot(q);
		if (v < 0.0 || u + v > 1.0) { return std::nullopt; }
		if (auto t = f * edge2.dot(q); t > EPSILON) { return { t }; }
		return std::nullopt;
	}

	std::optional<Vec3> GetNormalAt() const {
		const Vec3 edge1 = v1 - v0; // local normal unaffected by translation
		const Vec3 edge2 = v2 - v0;
		Vec3 n = edge1.cross(edge2);
        return n.normalize();
	}

	std::optional<HitInfo> GetHitInfoAt(const Rayon& ray, const size_t index) const {
		if (auto intersection = Intersect(ray); intersection) {
			const Vec3 hitPoint = ray.pointAtDistance(intersection.value());
			const Vec3 normal = GetNormalAt().value();
            return HitInfo{
                .type = HitType::TRIANGLE,
				.distance = intersection.value(),
                .index = index,
                .material = material,
                .normal = normal,
				.hitPoint = hitPoint
            };
		}
		return std::nullopt;
	}

	Material GetMaterial() const { return material; }
};

class Model
{
private:
    std::vector<int> vertices;
	std::vector<Vec3> vertexPositions;
	Transform transform;
	Material material;
public:
	Model(const std::vector<int>& vertices, const Transform& transform = Transform(), const Material& material = Material(), const std::vector<Vec3>& vertexPositions = std::vector<Vec3>()) : vertices(vertices), transform(transform), material(material), vertexPositions(vertexPositions) {}

    std::vector<Triangle> GetTrianglesFromModel(const Material& overrideMaterial) const {
        std::vector<Triangle> triangles;
        for (size_t i = 0; i < vertices.size(); i += 3) {
            Vec3 v0 = vertexPositions[vertices[i]];
            Vec3 v1 = vertexPositions[vertices[i + 1]];
            Vec3 v2 = vertexPositions[vertices[i + 2]];
            triangles.emplace_back(Triangle(v0, v1, v2, overrideMaterial, transform));
        }
        return triangles;
	}

    std::optional<HitInfo> GetHitInfoAt(const Rayon& ray, const size_t index) const {
        std::optional<HitInfo> closestHit = std::nullopt;
        for (size_t i = 0; i < vertices.size(); i += 3) {
            Vec3 v0 = vertexPositions[vertices[i]];
            Vec3 v1 = vertexPositions[vertices[i + 1]];
            Vec3 v2 = vertexPositions[vertices[i + 2]];
            Triangle triangle(v0, v1, v2, material, transform);
            if (auto hitOpt = triangle.GetHitInfoAt(ray, index); hitOpt) {
                if (HitInfo hit = hitOpt.value(); !closestHit.has_value() || hit.isCloserThan(closestHit.value())) {
                    closestHit = hit;
                }
            }
        }
        return closestHit;
	}

    std::optional<double> Intersect(const Rayon& ray) const {
        std::optional<double> closestT = std::nullopt;
        for (size_t i = 0; i < vertices.size(); i += 3) {
            Vec3 v0 = vertexPositions[vertices[i]];
            Vec3 v1 = vertexPositions[vertices[i + 1]];
            Vec3 v2 = vertexPositions[vertices[i + 2]];
            Triangle triangle(v0, v1, v2, material, transform);
            if (auto tOpt = triangle.Intersect(ray); tOpt) {
                double t = tOpt.value();
                if (!closestT.has_value() || t < closestT.value()) {
                    closestT = t;
                }
            }
        }
        return closestT;
	}

	Transform GetTransform() const { return transform; }
	Material GetMaterial() const { return material; }

	void SetTransform(const Transform& t) { transform = t; }
	void SetMaterial(const Material& m) { material = m; }
};