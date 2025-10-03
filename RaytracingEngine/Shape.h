#pragma once

#include <vector>
#include <optional>
#include "Math.cpp"

class Shape {
private:
	Vec3 position;
	Vec3 color;
public:
	Shape(const Vec3& pos = Vec3(0, 0, 0), const Vec3& col = Vec3(1, 1, 1)) : position(pos), color(col) {}
    virtual ~Shape() = default;
	virtual std::optional<double> intersect(const Rayon& ray) const { return std::nullopt; }
		
	Vec3 getPosition() const { return position; }
	Vec3 getColor() const { return color; }
	void setPosition(const Vec3& pos) { position = pos; }
	void setColor(const Vec3& col) { color = col; }
};

class Sphere : public Shape {
private:
	double radius;
public:
	Sphere(const Vec3& pos = Vec3(0, 0, 0), double r = 1, const Vec3& col = Vec3(1, 1, 1)) : Shape(pos, col), radius(r) {}
	std::optional<double> intersect(const Rayon& ray) const override;

	double getRadius() const { return radius; }
	void setRadius(double r) { radius = r; }
};

class Plane : public Shape {
private:
	Vec3 normal;
public:
	Plane(const Vec3& pos = Vec3(0, 0, 0), const Vec3& norm = Vec3(0, 1, 0), const Vec3& col = Vec3(1, 1, 1)) : Shape(pos, col), normal(norm) {}
	std::optional<double> intersect(const Rayon& ray) const override;

	Vec3 getNormal() const { return normal; }
	void setNormal(const Vec3& norm) { normal = norm; }
};