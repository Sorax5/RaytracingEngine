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
			HitInfo hit = s.getHitInfoAt(ray, sphereIndex);
			if (hit.hasHitSomething() && hit.isCloserThan(closest)) {
				closest = hit;
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			const Plane& p = planes[planeIndex];
			HitInfo hit = p.getHitInfoAt(ray, planeIndex);
			if (hit.hasHitSomething() && hit.isCloserThan(closest)) {
				closest = hit;
			}
		}

		return closest;
	}

	bool intersectAnyBefore(const Rayon& ray, double maxDist) const {
		if (spheres.empty() && planes.empty())
		{
			return false;
		}

		for (std::size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			std::optional<double> intersection = spheres[sphereIndex].intersect(ray);
			if (intersection) {
				const double dist = intersection.value();
				if (dist > 0.0 && dist < maxDist) {
					return true;
				}
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			std::optional<double> intersection = planes[planeIndex].intersect(ray);
			if (intersection.has_value()) {
				double dist = intersection.value();
				if (dist > 0.0 && dist < maxDist) {
					return true;
				}
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
		Vec3 accumulatedColor = Vec3(0, 0, 0);
		int samples = 0;

		const double bias = 1e-6;
		const int aaCount = camera.antiAliasingAmount;

		for (int aa = 0; aa < aaCount; ++aa)
		{
			Vec3 color = Vec3(0, 0, 0);

			HitInfo closest = CalculatePixelDepth(x, y, aa > 0 && aaCount > 1);
			if (!closest.hasHitSomething()) {
				continue;
			}

			auto optNormal = getNormalOfHit(closest);
			if (!optNormal.has_value()) {
				continue;
			}
			Vec3 normal = optNormal.value().normalize();

			Vec3 viewDir = (camera.position - closest.hitPoint).normalize();
			double shininess = getShininessOfHit(closest);

			Vec3 specularAccum = Vec3(0, 0, 0);
			Vec3 incoming = Vec3(0, 0, 0);
			for (std::size_t li = 0; li < lights.size(); ++li) {
				const Light& light = lights[li];

				Vec3 Ldir = light.dirTo(closest.hitPoint);
				double NdotL = normal.dot(Ldir);
				if (NdotL <= 0.0) {
					continue;
				}
				NdotL = std::max(0.0, NdotL);

				double dist = light.distanceTo(closest.hitPoint);
				if (dist <= bias)
				{
					continue;
				}

				Rayon shadowRay = light.shadowRayFrom(closest.hitPoint, bias);
				if (intersectAnyBefore(shadowRay, dist - bias)) {
					continue;
				}

				incoming += light.contributionFrom(dist, NdotL);

				Vec3 half = (Ldir + viewDir).normalize();
				double NdotH = std::max(0.0, normal.dot(half));
				if (NdotH > 0.0) {
					double specFactor = std::pow(NdotH, shininess);
					Vec3 specContribution = light.emitted() * (1.0 / (dist * dist)) * specFactor;
					specularAccum += specContribution;
				}
			}

			color = getColorOfHit(closest) * incoming + specularAccum;
			accumulatedColor += color;
			samples += 1;
		}

		if (samples > 0) {
			accumulatedColor /= static_cast<double>(samples);
			return accumulatedColor;
		}

		return Vec3(0, 0, 0);
	}

	std::vector<Vec3> GenerateImage() {
		std::vector<Vec3> finalImage(camera.width * camera.height, Vec3(0, 0, 0));

		int width = static_cast<int>(camera.width);
		int height = static_cast<int>(camera.height);
		const int totalPixels = width * height;

		/*#ifdef _OPENMP
		#pragma omp parallel for schedule(static) 
		#endif*/
		for(int idx = 0; idx < totalPixels; ++idx) {
			int x = idx % width;
			int y = idx / width;
			finalImage[idx] = GeneratePixelAt(x, y);
		}
		
		return finalImage;
	}
};