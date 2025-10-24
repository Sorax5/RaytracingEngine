#pragma once

#include <vector>
#include <memory>
#include <limits>
#include "Shape.h"
#include "Light.h"
#include <unordered_map>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

class Scene {
private:
	std::vector<Sphere> spheres;
	std::vector<Plane> planes;
	std::vector<Light> lights;

	Camera camera;
public:
	Scene(Camera cam) : camera(cam) {
		const std::size_t pixelCount = camera.width * camera.height;
		this->spheres = std::vector<Sphere>();
		this->planes = std::vector<Plane>();
		this->lights = std::vector<Light>();
	}

	void addSphere(Sphere& sphere) {
		spheres.emplace_back(std::move(sphere));
	}
	void addPlane(Plane& plane) {
		planes.emplace_back(std::move(plane));
	}
	void addLight(Light& light) {
		lights.emplace_back(std::move(light));
	}

	const std::vector<Sphere>& getSpheres() const { return spheres; }
	const std::vector<Plane>& getPlanes() const { return planes; }
	const std::vector<Light>& getLights() const { return lights; }

	const Camera& getCamera() const { return camera; }

	std::size_t getPixelIndex(std::size_t x, std::size_t y) const {
		return y * camera.width + x;
	}

	std::optional<Vec3> getNormalOfHit(const HitInfo& hit) const {
		if (!hit.index.has_value()) return std::nullopt;
		switch (hit.type) {
			case HitType::SPHERE:
				return spheres[hit.index.value()].getNormalAt(hit.hitPoint);
			case HitType::PLANE:
				return planes[hit.index.value()].getNormalAt();
			default:
				return std::nullopt;
		}
	}

	Vec3 getColorOfHit(const HitInfo& hit) const {
		if (!hit.index.has_value()) return Vec3(0,0,0);
		switch (hit.type)
		{
			case HitType::SPHERE:
				return spheres[hit.index.value()].getMaterial().color;
			case HitType::PLANE:
				return planes[hit.index.value()].getMaterial().color;
			default:
				return Vec3(0, 0, 0);
		}
	}

	double getShininessOfHit(const HitInfo& hit) const {
		if (!hit.index.has_value()) return 32.0;
		switch (hit.type)
		{
			case HitType::SPHERE:
				return spheres[hit.index.value()].getMaterial().shininess;
			case HitType::PLANE:
				return planes[hit.index.value()].getMaterial().shininess;
			default:
				return 32.0;
		}
	}

	HitInfo intersectClosest(const Rayon& ray) const {
		HitInfo closest{ std::numeric_limits<double>::infinity(), HitType::NONE, std::nullopt, Vec3() };

		for (std::size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			const Sphere& s = spheres[sphereIndex];
			HitInfo hit = s.getHitInfoAt(ray, static_cast<int>(sphereIndex));
			if (hit.hasHitSomething() && hit.isCloserThan(closest)) {
				closest = hit;
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			const Plane& p = planes[planeIndex];
			HitInfo hit = p.getHitInfoAt(ray, static_cast<int>(planeIndex));
			if (hit.hasHitSomething() && hit.isCloserThan(closest)) {
				closest = hit;
			}
		}

		return closest;
	}

	bool intersectAnyBefore(const Rayon& ray, double maxDist) const {
		for (std::size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			auto t = spheres[sphereIndex].intersect(ray);
			if (t.has_value()) {
				double dist = t.value();
				if (dist > 0.0 && dist < maxDist) return true;
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			auto t = planes[planeIndex].intersect(ray);
			if (t.has_value()) {
				double dist = t.value();
				if (dist > 0.0 && dist < maxDist) return true;
			}
		}

		return false;
	}

	HitInfo CalculatePixelDepth(std::size_t x, std::size_t y, bool aa) const {
		Rayon ray = camera.getRay(x, y, aa);
		HitInfo closest = intersectClosest(ray);
		return closest;
	}

	Vec3 GeneratePixelAt(int x, int y) const {
		std::vector<Vec3> colors;
		int pixelIndex = getPixelIndex(x, y);
		const double bias = 1e-3;

		for (size_t aa = 0; aa < camera.antiAliasingAmount; aa++)
		{
			Vec3 color = Vec3(0, 0, 0);

			HitInfo closest = CalculatePixelDepth(x, y, aa > 0 && camera.antiAliasingAmount > 1);
			if (!closest.hasHitSomething()) {
				continue;
			}

			std::optional<Vec3> optNormal = getNormalOfHit(closest);
			if (!optNormal.has_value()) {
				continue;
			}
			Vec3 normal = optNormal.value().normalize();

			Vec3 viewDir = (camera.position - closest.hitPoint).normalize();
			double shininess = getShininessOfHit(closest);

			Vec3 specularAccum = Vec3(0, 0, 0);
			Vec3 incoming = Vec3(0, 0, 0);

			for (const Light& light : lights) {
				double dist = light.distanceTo(closest.hitPoint);
				if (dist <= bias) {
					continue;
				}
				Vec3 Ldir = light.dirTo(closest.hitPoint);

				Rayon shadowRay = light.shadowRayFrom(closest.hitPoint, bias);
				if (intersectAnyBefore(shadowRay, dist - bias)) {
					continue;
				}

				double NdotL = std::max(0.0, normal.dot(Ldir));
				if (NdotL <= 0.0) {
					continue;
				}

				incoming += light.contributionFrom(dist, NdotL);
				Vec3 half = (Ldir + viewDir).normalize();
				double NdotH = std::max(0.0, normal.dot(half));
				double specFactor = std::pow(NdotH, shininess);
				Vec3 specContribution = light.emitted() * (1.0 / (dist * dist)) * specFactor;
				specularAccum += specContribution;
			}

			color = getColorOfHit(closest) * incoming + specularAccum;
			colors.push_back(color);
		}

		if (!colors.empty()) {
			Vec3 accumulatedColor = Vec3(0, 0, 0);
			for (const Vec3& col : colors) {
				accumulatedColor += col;
			}
			accumulatedColor /= static_cast<double>(colors.size());
			return accumulatedColor;
		}

		return Vec3(0, 0, 0);
	}

	std::vector<Vec3> GenerateImage() {
		std::vector<Vec3> finalImage(camera.width * camera.height, Vec3(0, 0, 0));

		int width = static_cast<int>(camera.width);
		int height = static_cast<int>(camera.height);
		const int totalPixels = width * height;

		#pragma omp parallel for schedule(dynamic) default(shared) firstprivate(width, height)
		for(int idx = 0; idx < totalPixels; ++idx) {
			int x = idx % width;
			int y = idx / width;
			finalImage[idx] = GeneratePixelAt(x, y);
		}
		
		return finalImage;
	}
};