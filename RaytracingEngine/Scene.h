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

	std::vector<Vec3> generateColormap() const {
		std::vector<Vec3> colorMap = std::vector<Vec3>(camera.width * camera.height, Vec3(0, 0, 0));


		for (int y = 0; y < camera.height; y++)
		{
			for (int x = 0; x < camera.width; x++) {
				int pixelIndex = getPixelIndex(x, y);

				HitInfo hit = depthMap[pixelIndex];
				if (hit.isValid())
				{
					double depth = 1 - (hit.distance - camera.nearPlaneDistance) / (camera.farPlaneDistance - camera.nearPlaneDistance);
					Vec3 color;
					if (hit.type == HitInfo::SPHERE)
					{
						color = spheres[hit.index]->getMaterial().color * depth;
					}
					else if (hit.type == HitInfo::PLANE)
					{
						color = planes[hit.index]->getMaterial().color * depth;
					}
					colorMap[pixelIndex] = color;
				}
			}
		}
		return colorMap;
	}
};