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
	double specular = 1.0;

	double refractiveIndex = 1.5;
	double transparency = 0.0;
	Vec3 absorption = Vec3(0.0, 0.0, 0.0);
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
		for (int i = 1; i < intersections.size(); i++)
		{
			HitInfo info = intersections[i];
			if (closestIntersection.isCloserThan(info))
			{
				continue;
			}
			closestIntersection = info;
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
	Sphere(double r = 1.0, const Vec3& pos = Vec3(0, 0, 0), const Vec3& col = Vec3(1, 1, 1)) : radius(r) {
		transform.position = pos;
		transform.rotation = Vec3(0, 0, 0);
		transform.scale = Vec3(1, 1, 1);
		material.color = col;
	}

	std::optional<double> intersect(const Rayon& ray) const {
		Vec3 oc = ray.origin - transform.position;

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

	std::optional<Vec3> getNormalAt(const Vec3& point) const {
		Vec3 normal = (point - transform.position).normalize();
		return normal;
	}

	std::optional<HitInfo> getHitInfoAt(const Rayon& ray, size_t index) const {
		std::optional<double> intersection = intersect(ray);
		if (intersection.has_value())
		{
			Vec3 hitPoint = ray.pointAtDistance(intersection.value());
			Vec3 normal = getNormalAt(hitPoint).value();
			return HitInfo{
				intersection.value(),
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
};

class Plane {
private:
	Vec3 normal;
	Transform transform;
	Material material;
public:
	Plane(const Vec3& pos = Vec3(0, 1, 0), const Vec3& norm = Vec3(0, 1, 0), const Vec3& col = Vec3(1, 1, 1))
		: normal(norm.normalize()) {
		transform.position = pos;
		transform.rotation = Vec3(0, 0, 0);
		transform.scale = Vec3(1, 1, 1);
		material.color = col;
	}

	std::optional<double> intersect(const Rayon& ray) const {
		double denom = normal.dot(ray.direction);
		if (std::abs(denom) > 1e-6) {
			Vec3 p0l0 = transform.position - ray.origin;
			double t = p0l0.dot(normal) / denom;
			if (t >= 0) {
				return { t };
			}
		}
		return std::nullopt;
	}

	std::optional<Vec3> getNormalAt() const {
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
};