#pragma once

#include <vector>
#include <optional>
#include "Math.cpp"

struct Transform {
	Vec3 position;
	Vec3 rotation;
	Vec3 scale;
};

struct Material {
	Vec3 color;
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
		Vec3 normal = (point - transform.position);
		return normal;
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
	Plane(const Vec3& pos = Vec3(0, 1, 0), const Vec3& norm = Vec3(0, 0, 0), const Vec3& col = Vec3(1, 1, 1)) : normal(norm) {
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

	std::optional<Vec3> getNormalAt(const Vec3& point) const {
		return normal;
	}

	Vec3 getNormal() const { return normal; }
	void setNormal(const Vec3& norm) { normal = norm; }
	Material getMaterial() const { return material; }
	Transform getTransform() const { return transform; }
};