#pragma once

#include "Math.h"
#include <functional>
#include <algorithm>

struct Light {
	Vec3 position;
	Vec3 color;
	double intensity;
	Light(const Vec3& position, const Vec3& color, const double& intensity)
		: position(position), color(color), intensity(intensity) {
	}

	Light() : position(Vec3(0, 0, 0)), color(Vec3(1, 1, 1)), intensity(1.0) {
	}

	Vec3 toLightDirection(const Vec3& point) const {
		return position - point;
	}

	double distanceTo(const Vec3& point) const {
		return toLightDirection(point).length();
	}

	Vec3 dirTo(const Vec3& point) const {
		const double EPS = 1e-12;
		Vec3 v = toLightDirection(point);
		double len = v.length();
		if (len <= EPS) return Vec3(0, 0, 0);
		return v / len;
	}

	Rayon shadowRayFrom(const Vec3& hitPoint, double bias) const {
		Vec3 Ldir = dirTo(hitPoint);
		return Rayon(hitPoint + Ldir * bias, Ldir);
	}

	Vec3 emitted() const {
		return color * intensity;
	}

	Vec3 contributionFrom(double dist, double NdotL) const {
		const double EPS = 1e-12;
		if (dist <= EPS) return Vec3(0, 0, 0);
		if (NdotL <= 0.0) return Vec3(0, 0, 0);
		double attenuation = 1.0 / (dist * dist);
		return emitted() * (attenuation * NdotL);
	}
};

