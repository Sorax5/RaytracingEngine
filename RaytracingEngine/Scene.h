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

    Vec3 directLightning(const HitInfo& hit, const Vec3& viewDir, const double bias) const {
        const Material& material = hit.material;

        Vec3 normal = hit.normal;
        if (normal.dot(viewDir) < 0.0) {
            normal = -normal;
        }

        auto diffuseAccumulation = Vec3{0,0,0};
        auto specularAccumlation = Vec3{0,0,0};

        for (const auto& light : lights) {
            const Vec3 lightToHit = light.dirTo(hit.hitPoint);
            const double normalDotLightHit = std::max(0.0, normal.dot(lightToHit));
            if (normalDotLightHit <= 0.0)
            {
	            continue;
            }

            const double distanceToLight = light.distanceTo(hit.hitPoint);
            if (distanceToLight <= bias)
            {
	            continue;
            }

            Rayon shadowRay{ hit.hitPoint + lightToHit * bias, lightToHit };
            const double transmittance = computeTransmittance(shadowRay, distanceToLight - bias, bias);
            if (transmittance <= bias)
            {
	            continue;
            }

            diffuseAccumulation += light.contributionFrom(distanceToLight, normalDotLightHit) * transmittance;

            if (material.transparency <= 0.0 && material.specular > 0.0) {
                Vec3 halfVector = (lightToHit + viewDir).normalize();
                double NdotH = std::max(0.0, normal.dot(halfVector));
                if (NdotH > 0.0) {
                    const double specFactor = std::pow(NdotH, material.shininess);
                    specularAccumlation += light.contributionFrom(distanceToLight, specFactor) * transmittance;
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

        constexpr double etaI = 1.0;
        const double etaT = material.refractiveIndex;
        const double f0 = std::pow((etaT - etaI) / (etaT + etaI), 2.0);
        double fresnelAmount = fresnel(cosTheta, f0);

        double transparency = std::clamp(material.transparency, 0.0, 1.0);

        Vec3 localLight = directLightning(hit, viewDir, bias);
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
				fresnelAmount = 1.0; // Total reflection interne
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
    }

    void AddSphere(Sphere& sphere) { spheres.emplace_back(sphere); }
    void AddPlane(Plane& plane) { planes.emplace_back(plane); }
    void AddLight(Light& light) { lights.emplace_back(light); }

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
            if (auto hitOpt = planes[planeIndex].getHitInfoAt(ray, planeIndex); hitOpt) {
                if (HitInfo hit = hitOpt.value(); !closest.has_value() || hit.isCloserThan(closest.value())) {
                    closest = hit;
                }
            }
        }

        return closest;
    }

    bool IntersectAnyBefore(const Rayon& ray, const double maxDist) const {
        if (spheres.empty() && planes.empty()) {
            return false;
        }

        for (auto sphere : spheres) {
            if (auto intersection = sphere.Intersect(ray)) {
                if (const double dist = intersection.value(); dist > 0.0 && dist < maxDist) {
                    return true;
                }
            }
        }

        for (auto plane : planes) {
            if (auto intersection = plane.intersect(ray)) {
                if (const double dist = intersection.value(); dist > 0.0 && dist < maxDist) {
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

        const int aaCount = camera.antiAliasingAmount;

        for (int aa = 0; aa < aaCount; ++aa)
        {
	        constexpr double bias = 1e-6;
	        if (auto color = GenerateAntiAliasing(x, y, aa > 0 && aaCount > 1, bias)) {
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