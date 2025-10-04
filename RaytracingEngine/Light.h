#pragma once

#include "Math.cpp"

struct Light {
	Vec3 position;
	Vec3 color;
	double intensity;
	Light(const Vec3& position, const Vec3& color, const double& intensity)
		: position(position), color(color), intensity(intensity) {
	}

	Light() : position(Vec3(0, 0, 0)), color(Vec3(1, 1, 1)), intensity(1.0) {
	}
};

