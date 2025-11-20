#pragma once

#include "Math.h"
#include <functional>

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
		static constexpr double EPS = 1e-12;
		const Vec3 v = position - point;

		const double nonSquaredLen = v.x * v.x + v.y * v.y + v.z * v.z;
		const double esp2 = EPS * EPS;
		if (nonSquaredLen <= esp2)
		{
			return Vec3{ 0, 0, 0 };
		}

		const double invLen = 1.0 / std::sqrt(nonSquaredLen);
		return invLen * v;
	}

	Rayon shadowRayFrom(const Vec3& hitPoint, const double bias) const {
		const Vec3 Ldir = dirTo(hitPoint);
		return Rayon{
			hitPoint + Ldir * bias,
			Ldir
		};
	}

	Vec3 emitted() const {
		return color * intensity;
	}

	Vec3 contributionFrom(const double dist, const double NdotL) const {
		const double EPS = 1e-12;
		if (dist <= EPS || NdotL <= 0.0) {
			return Vec3{ 0, 0, 0 };
		}
		return emitted() * (1.0 / (dist * dist) * NdotL);
	}
};

