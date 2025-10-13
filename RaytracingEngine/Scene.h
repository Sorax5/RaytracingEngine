#pragma once

#include <vector>
#include <memory>
#include "Shape.h"
#include "Light.h"
#include <unordered_map>
#include <iostream>


struct HitInfo {
	double distance;
	enum { NONE, SPHERE, PLANE } type;
	int index;

	bool isValid() const {
		return type != NONE && index != -1 && distance >= 0.001f;
	}

	bool operator<(const HitInfo& other) const {
		return distance < other.distance;
	}

	bool operator<=(const HitInfo& other) const {
		return distance <= other.distance;
	}

	bool operator>(const HitInfo& other) const {
		return distance > other.distance;
	}

	bool operator>=(const HitInfo& other) const {
		return distance >= other.distance;
	}
};

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

	int getPixelIndex(int x, int y) const {
		return y * camera.width + x;
	}

	std::vector<HitInfo> getAllIntersections(const Rayon& ray) const
	{
		int size = spheres.size() + planes.size();
		std::vector<HitInfo> intersections = std::vector<HitInfo>(size, { -1, HitInfo::NONE, -1 });

		for (int sphereIndex = 0; sphereIndex < spheres.size(); sphereIndex++) {
			const std::unique_ptr<Sphere>& shape = spheres[sphereIndex];
			std::optional<double> intersection = shape->intersect(ray);
			if (intersection.has_value()) {
				intersections[sphereIndex] = { intersection.value(), HitInfo::SPHERE, sphereIndex };
			}
		}

		for (int planeIndex = 0; planeIndex < planes.size(); planeIndex++)
		{
			const std::unique_ptr<Plane>& shape = planes[planeIndex];
			std::optional<double> intersection = shape->intersect(ray);
			if (intersection.has_value()) {
				intersections[spheres.size() + planeIndex] = { intersection.value(), HitInfo::PLANE, planeIndex };
			}
		}

		return intersections;
	}

	HitInfo getClosestIntersection(const std::vector<HitInfo>& intersections) const
	{
		HitInfo closestIntersection = { std::numeric_limits<double>::max(), HitInfo::NONE, -1 };
		for (int i = 0; i < intersections.size(); i++)
		{
			HitInfo info = intersections[i];
			if(!info.isValid())
			{
				continue;
			}

			if(info < closestIntersection)
			{
				closestIntersection = info;
			}

		}

		return closestIntersection;
	}

	void generateDepthmap() {
		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++)
			{
				Vec3 position = Vec3(x, y, 0);
				Rayon ray = camera.getRay(x, y);
				std::vector<HitInfo> intersections = getAllIntersections(ray);
				HitInfo closest = getClosestIntersection(intersections);
				if (closest.isValid())
				{
					this->depthMap[getPixelIndex(x, y)] = closest;
				}
			}

		}
	}

	void generateColormap() {
		std::vector<Vec3> colorMap = std::vector<Vec3>(camera.width * camera.height, Vec3(0, 0, 0));


		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);

				HitInfo hit = depthMap[pixelIndex];
				if (hit.isValid())
				{
					Vec3 color;
					if (hit.type == HitInfo::SPHERE)
					{
						color = spheres[hit.index]->getMaterial().color;
					}
					else if (hit.type == HitInfo::PLANE)
					{
						color = planes[hit.index]->getMaterial().color;
					}
					colorMap[pixelIndex] = color;
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
				if (hit.isValid())
				{
					Rayon ray = camera.getRay(x, y);
					Vec3 pointAtIntersection = ray.pointAtDistance(hit.distance);
					Vec3 normal;
					if (hit.type == HitInfo::SPHERE)
					{
						normal = spheres[hit.index]->getNormalAt(pointAtIntersection).value().normalize();
					}
					else if (hit.type == HitInfo::PLANE)
					{
						normal = planes[hit.index]->getNormalAt(pointAtIntersection).value().normalize();
					}
					normalMap[pixelIndex] = (normal + Vec3(1, 1, 1)) * 0.5f;
				}
			}
		}
		
		this->normalMap = normalMap;
	}

	void generateLightmap() {
		std::vector<Vec3> lightMap = std::vector<Vec3>(camera.width * camera.height, Vec3(0, 0, 0));
		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);
				HitInfo hit = depthMap[pixelIndex];
				if (hit.isValid())
				{
					Rayon ray = camera.getRay(x, y);
					Vec3 pointAtIntersection = ray.pointAtDistance(hit.distance);
					Vec3 normal;

					if (hit.type == HitInfo::SPHERE)
					{
						normal = spheres[hit.index]->getNormalAt(pointAtIntersection).value().normalize();
					}
					else if (hit.type == HitInfo::PLANE)
					{
						normal = planes[hit.index]->getNormalAt(pointAtIntersection).value().normalize();
					}

					Vec3 color = Vec3(0, 0, 0);
					for (const std::unique_ptr<Light>& light : lights)
					{
						Vec3 lightDir = (light->position - pointAtIntersection).normalize();

						double lightDistance = (light->position - pointAtIntersection).length();
						Rayon shadowRay(pointAtIntersection + normal * 0.001, lightDir);

						std::vector<HitInfo> shadowIntersections = getAllIntersections(shadowRay);
						HitInfo shadowHit = getClosestIntersection(shadowIntersections);

						bool inShadow = shadowHit.isValid() && shadowHit.distance < lightDistance;

						if (!inShadow) {
							double diff = std::max(normal.dot(lightDir), 0.0);
							color = color + light->color * light->intensity * diff;
						}
					}
					lightMap[pixelIndex] = color;
				}
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