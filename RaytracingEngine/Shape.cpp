// create abstract class Shape
#include "Shape.h"

std::optional<double> Shape::intersect(const Rayon& ray) const {
	return std::nullopt;
}

std::optional<double> Sphere::intersect(const Rayon& ray) const
{
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

std::optional<double> Plane::intersect(const Rayon& ray) const
{
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