#pragma once

#include <vector>
#include <memory>
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
		this->depthMap = std::vector<HitInfo>(camera.width * camera.height, { -1, HitInfo::NONE, -1 });
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

	const std::vector<Vec3>& getDepthMapValues() const {
		static std::vector<Vec3> depthValues;
		depthValues.assign(depthMap.size(), Vec3(0, 0, 0));

		for (int i = 0; i < depthMap.size(); i++) {
			const HitInfo& hit = depthMap[i];
			if (hit.hasHitSomething()) {
				double depthValue = hit.normalizedDistance(camera);
				depthValues[i] = Vec3(depthValue);
			}
		}
		return depthValues;
	}

	int getPixelIndex(int x, int y) const {
		return y * camera.width + x;
	}

	std::optional<Vec3> getNormalOfHit(const HitInfo& hit) const {
		switch (hit.type) {
			case HitInfo::SPHERE:
				return spheres[hit.index]->getNormalAt(hit.hitPoint);
			case HitInfo::PLANE:
				return planes[hit.index]->getNormalAt(hit.hitPoint);
			default:
				return std::nullopt;
		}
	}

	Vec3 getColorOfHit(const HitInfo& hit) const {
		switch (hit.type)
		{
			case HitInfo::SPHERE:
				return spheres[hit.index]->getMaterial().color;
			case HitInfo::PLANE:
				return planes[hit.index]->getMaterial().color;
			default:
				return Vec3(0, 0, 0);
		}
	}

	std::vector<HitInfo> getIntersections(const Rayon& ray) const
	{
		int size = spheres.size() + planes.size();
		std::vector<HitInfo> intersections = std::vector<HitInfo>(size, { -1, HitInfo::NONE, -1, Vec3()});

		for (int sphereIndex = 0; sphereIndex < spheres.size(); sphereIndex++) {
			const std::unique_ptr<Sphere>& shape = spheres[sphereIndex];
			intersections[sphereIndex] = shape->getHitInfoAt(ray, sphereIndex);
		}

		for (int planeIndex = 0; planeIndex < planes.size(); planeIndex++)
		{
			const std::unique_ptr<Plane>& shape = planes[planeIndex];
			intersections[spheres.size() + planeIndex] = shape->getHitInfoAt(ray, planeIndex);
		}

		return intersections;
	}

	void generateDepthmap() {
		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++)
			{
				const int idx = getPixelIndex(x, y);
				depthMap[idx] = { -1, HitInfo::NONE, -1, Vec3() };

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
		std::vector<Vec3> colorMap = std::vector<Vec3>(camera.width * camera.height, Vec3(1, 0, 1));

		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);

				HitInfo hit = depthMap[pixelIndex];
				if (hit.hasHitSomething())
				{
					colorMap[pixelIndex] = getColorOfHit(hit);
				}
			}
		}
		
		this->colorMap = colorMap;
	}

	void generateNormalmap() {
		std::vector<Vec3> normalMap = std::vector<Vec3>(camera.width * camera.height, Vec3(0, 0, 0));
		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);
				HitInfo hit = depthMap[pixelIndex];
				if(!hit.hasHitSomething()) {
					continue;
				}

				Vec3 normal = getNormalOfHit(hit).value().normalize();
				normalMap[pixelIndex] = normal;
			}
		}
		
		this->normalMap = normalMap;
	}

	void generateLightmap() {
		std::vector<Vec3> lightMap(camera.width * camera.height, Vec3(0, 0, 0));

		const double bias = 1e-3;           // décalage pour éviter auto-ombre
		const double ambientFactor = 0.2;  // ← ajuste pour ajouter lumière ambiante

		for (int y = 0; y < camera.height; y++) {
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);
				const HitInfo& pixelDepth = depthMap[pixelIndex];
				if (!pixelDepth.hasHitSomething()) {
					lightMap[pixelIndex] = Vec3(0, 0, 0);
					continue;
				}

				// récupère la normale géométrique et la normalise
				HitInfo hitCopy = pixelDepth;
				std::optional<Vec3> optNormal = getNormalOfHit(hitCopy);
				if (!optNormal.has_value()) {
					lightMap[pixelIndex] = Vec3(0, 0, 0);
					continue;
				}
				Vec3 normal = optNormal.value().normalize();

				// lighting commence par une composante ambiante
				Vec3 lighting = Vec3(ambientFactor, ambientFactor, ambientFactor);

				for (const std::unique_ptr<Light>& light : lights) {
					Vec3 toLight = (light->position - pixelDepth.hitPoint);
					double lightDistance = toLight.length();
					Vec3 lightDir = (lightDistance > 0.0) ? (toLight / lightDistance) : Vec3(0, 0, 0);

					Rayon shadowRay(pixelDepth.hitPoint + normal * bias, lightDir);
					std::vector<HitInfo> shadowIntersections = getIntersections(shadowRay);
					HitInfo shadowHit = HitInfo::getClosestIntersection(shadowIntersections);

					bool inShadow = shadowHit.hasHitSomething() &&
						shadowHit.distance > bias &&
						shadowHit.distance < (lightDistance - bias);

					if (inShadow) {
						continue;
					}

					double diff = std::max(normal.dot(lightDir), 0.0);

					// Atténuation simple (tu peux ajuster les coefficients)
					double attenuation = 1.0 / (1.0 + 0.1 * lightDistance + 0.01 * lightDistance * lightDistance);

					lighting = lighting + light->color * (light->intensity * diff * attenuation);
				}

				lightMap[pixelIndex] = lighting;
			}
		}

		this->lightMap = lightMap;
	}

	std::vector<Vec3> combineMaps() {
		std::vector<Vec3> combinedMap = std::vector<Vec3>(camera.width * camera.height, Vec3(0, 0, 0));
		for (int i = 0; i < combinedMap.size(); i++) {
			combinedMap[i] = colorMap[i] * lightMap[i];
		}
		return combinedMap;
	}
};