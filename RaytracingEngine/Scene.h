#pragma once

#include <vector>
#include <memory>
#include <limits>
#include "Shape.h"
#include "Light.h"
#include <unordered_map>
#include <iostream>
#include <algorithm>

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

	std::optional<HitInfo> intersectClosest(const Rayon& ray) const {
		std::optional<HitInfo> closest = std::nullopt;

		for (std::size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			auto hitOpt = spheres[sphereIndex].getHitInfoAt(ray, sphereIndex);
			if (!hitOpt.has_value()) {
				continue;
			}
			HitInfo hit = hitOpt.value();

			if (!closest.has_value() || hit.isCloserThan(closest.value())) {
				closest = hit;
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			auto hitOpt = planes[planeIndex].getHitInfoAt(ray, planeIndex);
			if (!hitOpt.has_value()) {
				continue;
			}
			HitInfo hit = hitOpt.value();

			if (!closest.has_value() || hit.isCloserThan(closest.value())) {
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
			if (auto intersection = spheres[sphereIndex].intersect(ray)) {
				const double dist = intersection.value();
				if (dist > 0.0 && dist < maxDist) {
					return true;
				}
			}
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			if (auto intersection = planes[planeIndex].intersect(ray)) {
				double dist = intersection.value();
				if (dist > 0.0 && dist < maxDist) {
					return true;
				}
			}
		}

		return false;
	}

	std::optional<HitInfo> CalculatePixelDepth(std::size_t x, std::size_t y, bool aa) const {
		Rayon ray = camera.getRay(x, y, aa);
		return intersectClosest(ray);
	}

	Vec3 GeneratePixelAt(int x, int y) const {
		Vec3 accumulatedColor = Vec3(0, 0, 0);
		int samples = 0;

		const double bias = 1e-6;
		const int aaCount = camera.antiAliasingAmount;

		for (int aa = 0; aa < aaCount; ++aa)
		{
			bool isAAActive = aa > 0 && aaCount > 1;
			
			if (auto color = GenerateAntiAliasing(x, y, isAAActive, bias))
			{
				accumulatedColor += color.value();
				samples += 1;
			}
		}

		if (samples > 0) {
			accumulatedColor /= static_cast<double>(samples);
			return accumulatedColor;
		}

		return Vec3(0, 0, 0);
	}

	std::optional<Vec3> GenerateAntiAliasing(size_t x, size_t y, bool isActive, double bias) const {
		auto hitOpt = CalculatePixelDepth(x, y, isActive);
		if (!hitOpt) {
			return std::nullopt;
		}
		const HitInfo& hit = *hitOpt;

		const Material& mat = hit.material;
		const Vec3& normal = hit.normal;

		Vec3 viewDir = (camera.position - hit.hitPoint).normalize();
		double shininess = mat.shininess;

		Vec3 specularAccum = Vec3(0, 0, 0);
		Vec3 incoming = Vec3(0, 0, 0);

		for (std::size_t li = 0; li < lights.size(); ++li) {
			const Light& light = lights[li];

			Vec3 Ldir = light.dirTo(hit.hitPoint).normalize();
			double NdotL = std::max(0.0, normal.dot(Ldir));
			if (NdotL <= 0.0) {
				continue;
			}

			double dist = light.distanceTo(hit.hitPoint);
			if (dist <= bias) {
				continue;
			}

			Rayon shadowRay = light.shadowRayFrom(hit.hitPoint, bias);
			if (intersectAnyBefore(shadowRay, dist - bias)) {
				continue;
			}

			Vec3 diffuse = light.contributionFrom(dist, NdotL);
			incoming += diffuse;

			Vec3 half = (Ldir + viewDir).normalize();
			double NdotH = std::max(0.0, normal.dot(half));
			if(NdotH <= 0.0) {
				continue;
			}

			double specFactor = std::pow(NdotH, shininess);
			Vec3 specContribution = light.emitted() * (1.0 / (dist * dist)) * specFactor;
			specularAccum += specContribution;
		}

		return mat.color * incoming + specularAccum;
	}

	std::vector<Vec3> GenerateImage() {
		std::vector<Vec3> finalImage(camera.width * camera.height, Vec3(0, 0, 0));

		int width = static_cast<int>(camera.width);
		int height = static_cast<int>(camera.height);
		const int totalPixels = width * height;

		#ifdef _OPENMP
		#pragma omp parallel for schedule(dynamic) 
		#endif
		for(int idx = 0; idx < totalPixels; ++idx) {
			int x = idx % width;
			int y = idx / width;
			finalImage[idx] = GeneratePixelAt(x, y);
		}

		return finalImage;
	}
};