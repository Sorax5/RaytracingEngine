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

	Vec3 calculateSpecular(const Vec3& lightDir, const Vec3& viewDir, const Vec3& normal) const {
		Vec3 reflectDir = (lightDir - normal * 2.0 * lightDir.dot(normal)).normalize();
		double spec = std::pow(std::max(viewDir.dot(reflectDir), 0.0), shininess);
		return Vec3(1, 1, 1) * spec;
	}
};

enum class HitType: unsigned char {
	NONE,
	SPHERE,
	PLANE
};

struct HitInfo {
    double distance;
	HitType type;
    std::optional<std::size_t> index;
    Vec3 hitPoint;

	bool hasHitSomething() const {
		return type != HitType::NONE && index != -1 && distance > 1e-4;
	}

	bool isCloserThan(const HitInfo& other) const {
		return distance < other.distance;
	}

	double normalizedDistance(const Camera& camera) const {
		return (distance - camera.nearPlaneDistance) / (camera.farPlaneDistance - camera.nearPlaneDistance);
	}

	static HitInfo getClosestIntersection(const std::vector<HitInfo>& intersections) {
		HitInfo closestIntersection = { std::numeric_limits<double>::max(), HitType::NONE, -1, Vec3() };
		for (int i = 0; i < intersections.size(); i++)
		{
			HitInfo info = intersections[i];
			if (!info.hasHitSomething())
			{
				continue;
			}

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

	HitInfo getHitInfoAt(const Rayon& ray, size_t index) const {
		HitInfo info = { -1, HitType::NONE, -1, Vec3() };

		std::optional<double> intersection = intersect(ray);
		if (intersection.has_value())
		{
			info.hitPoint = ray.pointAtDistance(intersection.value());
			info.distance = intersection.value();
		}

		info.type = HitType::SPHERE;
		info.index = index;
		return info;
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

	HitInfo getHitInfoAt(const Rayon& ray, int index) const {
		HitInfo info = { -1, HitType::NONE, -1, Vec3() };

		std::optional<double> intersection = intersect(ray);
		if (intersection.has_value())
		{
			info.hitPoint = ray.pointAtDistance(intersection.value());
			info.distance = intersection.value();
		}

		info.type = HitType::PLANE;
		info.index = index;
		return info;
	}

	Vec3 getNormal() const { return normal; }
	void setNormal(const Vec3& norm) { normal = norm.normalize(); }
	Material getMaterial() const { return material; }
	Transform getTransform() const { return transform; }
};