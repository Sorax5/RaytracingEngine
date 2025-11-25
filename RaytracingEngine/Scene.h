#pragma once

#include <vector>
#include "Shape.h"
#include "Light.h"
#include <algorithm>
#include <iostream>
#include <ranges>

#ifdef _OPENMP
#include <omp.h>
#endif

class Scene {
private:

    std::vector<Sphere> spheres;
    std::vector<Plane> planes;
	std::vector<Triangle> triangles;
	std::vector<Model> models;
    std::vector<Light> lights;

    Camera camera;
    int maxRecursion = 10;

    static double fresnel(const double cosTheta, const double F0) {
        return F0 + (1.0 - F0) * std::pow(1.0 - cosTheta, 5.0);
    }

    Vec3 backgroundColor(const Rayon& ray) const {
        double t = 0.5 * (ray.direction.normalize().y + 1.0);
        return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
    }

    double computeTransmittance(const Rayon& ray, const double maxDist, const double bias) const {
        double T = 1.0;
        double traveled = 0.0;
        Rayon r = ray;
        int safety = 64;


        while (safety-- > 0 && T > 1e-4 && traveled < maxDist) {
            auto hitOpt = IntersectClosest(r);
            if (!hitOpt)
            {
                break;
            }

            const HitInfo& hit = *hitOpt;
            const double t = hit.distance;
            if (t <= 0.0) {
                r.origin = r.origin + r.direction * (bias);
                traveled += bias;
                continue;
            }

            if (t <= bias) {
                r.origin = r.pointAtDistance(t) + r.direction * bias;
                traveled += t + bias;
                continue;
            }

            if (traveled + t >= maxDist)
            {
                break;
            }

            const double tr = std::clamp(hit.material.transparency, 0.0, 1.0);
            T *= tr;

            const Vec3 newOrigin = r.pointAtDistance(t) + r.direction * bias;
            traveled += t + bias;
            r.origin = newOrigin;
        }

        return std::clamp(T, 0.0, 1.0);
    }

    Vec3 directLightning(const HitInfo& hit, const Vec3& viewDir, const Vec3& normalIn, const double bias) const {
        const Material& material = hit.material;
        Vec3 normal = normalIn.normalize();

        auto diffuseAccumulation = Vec3{ 0,0,0 };
        auto specularAccumlation = Vec3{ 0,0,0 };

        for (const auto& light : lights) {
            Vec3 vecToLight = light.position - hit.hitPoint;
            const double distanceToLight = vecToLight.length();
            if (distanceToLight <= 0.0) continue;
            Vec3 lightToHit = vecToLight / distanceToLight;

            const double normalDotLightHit = std::max(0.0, normal.dot(lightToHit));
            if (normalDotLightHit <= 0.0)
            {
                continue;
            }

            if (distanceToLight <= bias)
            {
                continue;
            }

            Rayon shadowRay{ hit.hitPoint + normal * bias, lightToHit };
            const double transmittance = computeTransmittance(shadowRay, distanceToLight - bias, bias);
            if (transmittance <= bias)
            {
                continue;
            }

            Vec3 emitted = light.color * light.intensity;
            Vec3 contribution = emitted * (1.0 / (distanceToLight * distanceToLight)) * normalDotLightHit;

            diffuseAccumulation += contribution * transmittance;

            if (material.transparency <= 0.0 && material.specular > 0.0) {
                Vec3 halfVector = (lightToHit + viewDir).normalize();
                double NdotH = std::max(0.0, normal.dot(halfVector));
                if (NdotH > 0.0) {
                    const double specFactor = std::pow(NdotH, material.shininess);
                    // speculaire pondéré par la même attenuation et transmittance
                    specularAccumlation += (emitted * (1.0 / (distanceToLight * distanceToLight))) * specFactor * transmittance;
                }
            }
        }

        Vec3 diffuse = material.color * diffuseAccumulation;
        Vec3 specular = specularAccumlation * material.specular;
        return diffuse + specular;
    }

    std::optional<Vec3> TraceRay(const Rayon& traceRay, int recursionAmount, const double bias) const {
        if (recursionAmount >= maxRecursion) {
			return backgroundColor(traceRay); // ciel
        }

        const auto hitOpt = IntersectClosest(traceRay);
        if (!hitOpt) {
            return backgroundColor(traceRay);
        }

        const HitInfo hit = hitOpt.value();
        const Material& material = hit.material;

        const Vec3 incoming = traceRay.direction.normalize();
        const bool frontFace = hit.normal.dot(incoming) < 0.0;
        const Vec3 normal = frontFace ? hit.normal : -hit.normal;
        const Vec3 viewDir = -incoming;
        const double cosTheta = std::max(0.0, normal.dot(viewDir));

        static constexpr bool visualizeNormals = false;
        if (visualizeNormals) {
            if (!std::isfinite(hit.distance) ||
                !std::isfinite(hit.normal.x) || !std::isfinite(hit.normal.y) || !std::isfinite(hit.normal.z)) {
                return Vec3(1.0, 0.0, 1.0); // magenta = hit invalide
            }
            Vec3 n = normal.normalize();
            // mapping standard pour debug normals
            return Vec3((n.x * 0.5) + 0.5, (n.y * 0.5) + 0.5, (n.z * 0.5) + 0.5);
        }

        constexpr double etaI = 1.0;
        const double etaT = material.refractiveIndex;
        const double f0 = std::pow((etaT - etaI) / (etaT + etaI), 2.0);
        double fresnelAmount = fresnel(cosTheta, f0);

        double transparency = std::clamp(material.transparency, 0.0, 1.0);

        Vec3 localLight = directLightning(hit, viewDir, normal, bias);
        Vec3 finalLight{0,0,0};

        if (transparency < 1.0) {
            finalLight += localLight * (1.0 - transparency);
        }

        if (transparency > 0.0) {
            const double eta = frontFace ? (etaI / etaT) : (etaT / etaI);

            if (auto refractDir = incoming.refract(normal, eta); refractDir.length() > bias) {
                refractDir = refractDir.normalize();
                Rayon refractRay{ hit.hitPoint + refractDir * (bias * 1e2), refractDir };
                if (auto tc = TraceRay(refractRay, recursionAmount + 1, bias)) {
                    finalLight += tc.value() * (transparency * (1.0 - fresnelAmount));
                }
            } else {
				fresnelAmount = 1.0;
            }
        }

    	if (auto reflectiveness = (transparency > 0.0) ? fresnelAmount : material.specular; reflectiveness > bias) {
            Vec3 reflectDir = incoming.reflect(normal).normalize();
            Rayon reflectRay{ hit.hitPoint + reflectDir * bias, reflectDir };
            if (auto rc = TraceRay(reflectRay, recursionAmount + 1, bias)) {
                finalLight += rc.value() * reflectiveness;
            }
        }

        return finalLight;
    }
public:
    explicit Scene(const Camera& camera) : camera(camera) {
        const size_t pixelCount = camera.width * camera.height;
        this->spheres = std::vector<Sphere>();
        this->planes = std::vector<Plane>();
        this->lights = std::vector<Light>();
		this->triangles = std::vector<Triangle>();
    }

    void AddSphere(Sphere& sphere) { spheres.emplace_back(sphere); }
    void AddPlane(Plane& plane) { planes.emplace_back(plane); }
    void AddLight(Light& light) { lights.emplace_back(light); }
	void AddTriangle(Triangle& triangle) { triangles.emplace_back(triangle); }
	void AddModel(Model& model) { models.emplace_back(model); }

    size_t GetPixelIndex(const size_t x, const size_t y) const {
        return y * camera.width + x;
    }

    std::optional<HitInfo> IntersectClosest(const Rayon& ray) const {
        std::optional<HitInfo> closest = std::nullopt;

        for (size_t sphereIndex = 0; sphereIndex < spheres.size(); ++sphereIndex) {
            if (auto hitOpt = spheres[sphereIndex].GetHitInfoAt(ray, sphereIndex); hitOpt) {
                if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
                    closest = hit;
                }
            }
        }

        for (size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
            if (auto hitOpt = planes[planeIndex].GetHitInfoAt(ray, planeIndex); hitOpt) {
                if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
                    closest = hit;
                }
            }
        }

        for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
            if (auto hitOpt = triangles[triangleIndex].GetHitInfoAt(ray, triangleIndex); hitOpt) {
                if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
                    closest = hit;
                }
            }
        }

        for (int modelIndex = 0; modelIndex < models.size(); ++modelIndex)
        {
            if (auto hitOpt = models[modelIndex].GetHitInfoAt(ray, modelIndex); hitOpt)
            {
                
                if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
                    closest = hit;
                }
            }
        }

        return closest;
    }

    bool IntersectAnyBefore(const Rayon& ray, const double maxDist) const {
        if (spheres.empty() && planes.empty() && triangles.empty() && models.empty()) {
            return false;
        }

        auto within = [&](const double d) { return d > 0.0 && d < maxDist; };

        auto any_hit = [&](const auto& container) {
            return std::ranges::any_of(container, [&](const auto& obj) {
                if (auto distOpt = obj.Intersect(ray)) {
                    return within(*distOpt);
                }
                return false;
            });
        };

        return any_hit(spheres) || any_hit(planes) || any_hit(triangles) || any_hit(models);
    }

    std::optional<HitInfo> CalculatePixelDepth(const size_t x, const size_t y, const bool aa) const {
        const Rayon ray = camera.getRay(x, y, aa);
        return IntersectClosest(ray);
    }

    Vec3 GeneratePixelAt(const int x, const int y) const {
        auto accumulatedColor = Vec3{ 0,0,0 };
        int samples = 0;

        const int aaCount = camera.antiAliasingAmount;

        for (int aa = 0; aa < aaCount; ++aa)
        {
	        constexpr double bias = 1e-3;
	        if (auto color = GenerateAntiAliasing(x, y, aa > 0 && aaCount > 1, bias)) {
                accumulatedColor += color.value();
                samples += 1;
            }
        }

        if (samples > 0) {
            accumulatedColor /= samples;
            return accumulatedColor;
        }

        return Vec3{ 0, 0, 0 };
    }

    std::optional<Vec3> GenerateAntiAliasing(const size_t x, const size_t y, const bool isActive, const double bias) const {
        const Rayon ray = camera.getRay(x, y, isActive);
        return TraceRay(ray, 0, bias);
    }

    std::vector<Vec3> RenderImage() const {
        std::vector<Vec3> finalImage(camera.width * camera.height, Vec3(0, 0, 0));

        const int width = static_cast<int>(camera.width);
        const int height = static_cast<int>(camera.height);
        const int totalPixels = width * height;

        #ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic)
        #endif
        for (int idx = 0; idx < totalPixels; ++idx) {
            const int x = idx % width;
            const int y = idx / width;
            finalImage[idx] = GeneratePixelAt(x, y);
        }

        return finalImage;
    }
};