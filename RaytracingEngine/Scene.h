#pragma once

#include <vector>
#include <memory>
#include <limits>
#include "Shape.h"
#include "Light.h"
#include <unordered_map>
#include <iostream>

class Scene {
private:
	std::vector<std::unique_ptr<Sphere>> spheres;
	std::vector<std::unique_ptr<Plane>> planes;
	std::vector<std::unique_ptr<Light>> lights;

	std::vector<HitInfo> depthMap;
	std::vector<Vec3> normalMap;
	std::vector<Vec3> colorMap;
	std::vector<Vec3> lightMap;

	Camera camera;
public:
	Scene(Camera cam) : camera(cam) {
		const std::size_t pixelCount = camera.width * camera.height;
		this->depthMap = std::vector<HitInfo>(pixelCount, { std::numeric_limits<double>::infinity(), HitInfo::NONE, std::nullopt, Vec3() });
		this->spheres = std::vector<std::unique_ptr<Sphere>>();
		this->planes = std::vector<std::unique_ptr<Plane>>();
		this->lights = std::vector<std::unique_ptr<Light>>();
	}

	void addSphere(const Sphere& sphere) {
		spheres.push_back(std::make_unique<Sphere>(sphere));
	}
	void addPlane(const Plane& plane) {
		planes.push_back(std::make_unique<Plane>(plane));
	}
	void addLight(const Light& light) {
		lights.push_back(std::make_unique<Light>(light));
	}

	const std::vector<std::unique_ptr<Sphere>>& getSpheres() const { return spheres; }
	const std::vector<std::unique_ptr<Plane>>& getPlanes() const { return planes; }
	const std::vector<std::unique_ptr<Light>>& getLights() const { return lights; }

	const std::vector<Vec3>& getColorMap() const { return colorMap; }
	const std::vector<Vec3>& getNormalMap() const { return normalMap; }
	const std::vector<Vec3>& getLightMap() const { return lightMap; }

	const Camera& getCamera() const { return camera; }

	const std::vector<HitInfo>& getDepthMap() const { return depthMap; }

	std::vector<Vec3> getDepthMapValues() const {
		const std::size_t n = depthMap.size();
		std::vector<Vec3> depthValues(n, Vec3(0, 0, 0));
		for (std::size_t i = 0; i < n; ++i) {
			const HitInfo& hit = depthMap[i];
			if (hit.hasHitSomething()) {
				double depthValue = hit.normalizedDistance(camera);
				depthValues[i] = Vec3(depthValue, depthValue, depthValue);
			}
		}
		return depthValues;
	}

	std::size_t getPixelIndex(std::size_t x, std::size_t y) const {
		return y * camera.width + x;
	}

	std::optional<Vec3> getNormalOfHit(const HitInfo& hit) const {
		if (!hit.index.has_value()) return std::nullopt;
		switch (hit.type) {
			case HitInfo::SPHERE:
				return spheres[hit.index.value()]->getNormalAt(hit.hitPoint);
			case HitInfo::PLANE:
				return planes[hit.index.value()]->getNormalAt();
			default:
				return std::nullopt;
		}
	}

	Vec3 getColorOfHit(const HitInfo& hit) const {
		if (!hit.index.has_value()) return Vec3(0,0,0);
		switch (hit.type)
		{
			case HitInfo::SPHERE:
				return spheres[hit.index.value()]->getMaterial().color;
			case HitInfo::PLANE:
				return planes[hit.index.value()]->getMaterial().color;
			default:
				return Vec3(0, 0, 0);
		}
	}

	std::vector<HitInfo> getIntersections(const Rayon& ray) const
	{
		const std::size_t size = spheres.size() + planes.size();
		std::vector<HitInfo> intersections(size, { std::numeric_limits<double>::infinity(), HitInfo::NONE, std::nullopt, Vec3() });

		for (std::size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			const std::unique_ptr<Sphere>& shape = spheres[sphereIndex];
			intersections[sphereIndex] = shape->getHitInfoAt(ray, sphereIndex);
		}

		for (std::size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex)
		{
			const std::unique_ptr<Plane>& shape = planes[planeIndex];
			intersections[spheres.size() + planeIndex] = shape->getHitInfoAt(ray, planeIndex);
		}

		return intersections;
	}

	void generateDepthmap() {
		for (std::size_t y = 0; y < camera.height; ++y)
		{
			for (std::size_t x = 0; x < camera.width; ++x)
			{
				const std::size_t idx = getPixelIndex(x, y);
				depthMap[idx] = { std::numeric_limits<double>::infinity(), HitInfo::NONE, std::nullopt, Vec3() };

				Rayon ray = camera.getRay(x, y);
				std::vector<HitInfo> intersections = getIntersections(ray);
				HitInfo closest = HitInfo::getClosestIntersection(intersections);
				if (closest.hasHitSomething())
				{
					depthMap[idx] = closest;
				}
			}

		}
	}

	void generateColormap() {
		const std::size_t pixelCount = camera.width * camera.height;
		std::vector<Vec3> colorMapLocal(pixelCount, Vec3(1, 0, 1));

		for (std::size_t y = 0; y < camera.height; ++y)
		{
			for (std::size_t x = 0; x < camera.width; ++x) {
				std::size_t pixelIndex = getPixelIndex(x, y);

				HitInfo hit = depthMap[pixelIndex];
				if (hit.hasHitSomething())
				{
					colorMapLocal[pixelIndex] = getColorOfHit(hit);
				}
			}
		}
		
		this->colorMap = std::move(colorMapLocal);
	}

	void generateNormalmap() {
		const std::size_t pixelCount = camera.width * camera.height;
		std::vector<Vec3> normalMapLocal(pixelCount, Vec3(0, 0, 0));
		for (std::size_t y = 0; y < camera.height; ++y)
		{
			for (std::size_t x = 0; x < camera.width; ++x) {
				std::size_t pixelIndex = getPixelIndex(x, y);
				HitInfo hit = depthMap[pixelIndex];
				if(!hit.hasHitSomething()) {
					continue;
				}

				Vec3 normal = getNormalOfHit(hit).value().normalize();
				normalMapLocal[pixelIndex] = normal;
			}
		}
		
		this->normalMap = std::move(normalMapLocal);
	}

	void generateLightmap() {
		const std::size_t pixelCount = camera.width * camera.height;
		std::vector<Vec3> lightMapLocal(pixelCount, Vec3(0, 0, 0));

		const double bias = 1e-3;
		for (std::size_t y = 0; y < camera.height; ++y) {
			for (std::size_t x = 0; x < camera.width; ++x) {
				std::size_t pixelIndex = getPixelIndex(x, y);
				const HitInfo& pixelDepth = depthMap[pixelIndex];
				if (!pixelDepth.hasHitSomething()) {
					continue;
				}

				auto optNormal = getNormalOfHit(pixelDepth);
				if (!optNormal.has_value()) {
					continue;
				}
				Vec3 normal = optNormal.value().normalize();

				Vec3 incoming = Vec3(0, 0, 0);
				for (const std::unique_ptr<Light>& light : lights) {
					double dist = light->distanceTo(pixelDepth.hitPoint);
					if (dist <= 1e-6) continue;

					Vec3 Ldir = light->dirTo(pixelDepth.hitPoint);
					Rayon shadowRay = light->shadowRayFrom(pixelDepth.hitPoint, bias);

					std::vector<HitInfo> shadowIntersections = getIntersections(shadowRay);
					HitInfo shadowHit = HitInfo::getClosestIntersection(shadowIntersections);

					if (shadowHit.hasHitSomething() && shadowHit.distance < (dist - bias)) {
						continue;
					}

					double NdotL = std::max(0.0, normal.dot(Ldir));
					if (NdotL <= 0.0) continue;

					incoming += light->contributionFrom(dist, NdotL);
				}

				lightMapLocal[pixelIndex] = incoming;
			}
		}

		this->lightMap = std::move(lightMapLocal);
	}

	std::vector<Vec3> combineMaps() {
		const std::size_t pixelCount = camera.width * camera.height;
		std::vector<Vec3> combinedMap(pixelCount, Vec3(0, 0, 0));
		for (std::size_t i = 0; i < combinedMap.size(); ++i) {
			combinedMap[i] = colorMap[i] * lightMap[i];
		}
		return combinedMap;
	}
};