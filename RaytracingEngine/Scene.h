#pragma once

#include <vector>
#include "Shape.h"
#include "Light.h"
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

	size_t getPixelIndex(size_t x, size_t y) const {
		return y * camera.width + x;
	}

	std::optional<HitInfo> IntersectClosest(const Rayon& ray) const {
		std::optional<HitInfo> closest = std::nullopt;

		for (size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
			if (auto hitOpt = spheres[sphereIndex].getHitInfoAt(ray, sphereIndex); hitOpt) {
				if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
					closest = hit;
				}
			}
		}

		for (size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
			if (auto hitOpt = planes[planeIndex].getHitInfoAt(ray, planeIndex); hitOpt) {
				if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
					closest = hit;
				}
			}
		}

		return closest;
	}

	bool IntersectAnyBefore(const Rayon& ray, const double maxDist) const {
		if (spheres.empty() && planes.empty())
		{
			return false;
		}

		for (auto sphere : spheres)
		{
			if (auto intersection = sphere.intersect(ray)) 
			{
				if (const double dist = *intersection; dist > 0.0 && dist < maxDist) 
				{
					return true;
				}
			}
		}

		for (auto plane : planes)
		{
			if (auto intersection = plane.intersect(ray)) 
			{
				if (const double dist = *intersection; dist > 0.0 && dist < maxDist) 
				{
					return true;
				}
			}
		}

		return false;
	}

	std::optional<HitInfo> CalculatePixelDepth(const size_t x, const size_t y, const bool aa) const {
		const Rayon ray = camera.getRay(x, y, aa);
		return IntersectClosest(ray);
	}

	Vec3 GeneratePixelAt(const int x, const int y) const {
		auto accumulatedColor = Vec3{ 0,0,0 };
		int samples = 0;

		const double bias = 1e-6;
		const int aaCount = camera.antiAliasingAmount;

		for (int aa = 0; aa < aaCount; ++aa)
		{
			if (auto color = GenerateAntiAliasing(x, y, aa > 0 && aaCount > 1, bias))
			{
				accumulatedColor += color.value();
				samples += 1;
			}
		}

		if (samples > 0) {
			accumulatedColor /= static_cast<double>(samples);
			return accumulatedColor;
		}

		return Vec3{ 0, 0, 0 };
	}

	std::optional<Vec3> GenerateAntiAliasing(const size_t x, const size_t y, const bool isActive, const double bias) const {
		const auto hitOpt = CalculatePixelDepth(x, y, isActive);
		if (!hitOpt) {
			return std::nullopt;
		}
		const HitInfo& hit = *hitOpt;

		const Material& mat = hit.material;
		const Vec3& normal = hit.normal;

		const Vec3 viewDir = (camera.position - hit.hitPoint).normalize();
		const double shininess = mat.shininess;

		auto specularAccum = Vec3(0, 0, 0);
		auto incoming = Vec3(0, 0, 0);

		for (auto light : lights)
		{
			Vec3 Ldir = light.dirTo(hit.hitPoint).normalize();
			double NdotL = std::max(0.0, normal.dot(Ldir));
			if (NdotL <= 0.0) {
				continue;
			}

			const double dist = light.distanceTo(hit.hitPoint);
			if (dist <= bias) {
				continue;
			}

			if (Rayon shadowRay = light.shadowRayFrom(hit.hitPoint, bias); IntersectAnyBefore(shadowRay, dist - bias)) {
				continue;
			}

			const Vec3 diffuse = light.contributionFrom(dist, NdotL);
			incoming += diffuse;

			Vec3 half = (Ldir + viewDir).normalize();
			double NdotH = std::max(0.0, normal.dot(half));
			if (NdotH <= 0.0) {
				continue;
			}

			const double specFactor = std::pow(NdotH, shininess);
			const Vec3 specContribution = light.emitted() * (1.0 / (dist * dist)) * specFactor;
			specularAccum += specContribution;
		}

		return mat.color * incoming + specularAccum;
	}

	std::vector<Vec3> GenerateImage() const {
		std::vector<Vec3> finalImage(camera.width * camera.height, Vec3(0, 0, 0));

		const int width = static_cast<int>(camera.width);
		const int height = static_cast<int>(camera.height);
		const int totalPixels = width * height;

		#ifdef _OPENMP
		#pragma omp parallel for schedule(dynamic) 
		#endif
		for(int idx = 0; idx < totalPixels; ++idx) {
			const int x = idx % width;
			const int y = idx / width;
			finalImage[idx] = GeneratePixelAt(x, y);
		}

		return finalImage;
	}
};