#include "Math.cpp"
#include "Image.h"
#include "Light.h"
#include "Shape.h"
#include "Scene.h"
#include <vector>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 800

std::vector<Color> tonemap(const std::vector<Vec3>& pixels)
{
	std::vector<Color> colorPixels = std::vector<Color>(pixels.size());
	for (int i = 0; i < pixels.size(); i++) {
		colorPixels[i] = Color(
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].x * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].y * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].z * 255.0)))
		);
	}
	return colorPixels;
}

/*Vec3 calculateLighting(const Vec3& point, const Vec3& normal, const std::vector<std::unique_ptr<Light>>& lights, const std::vector<std::unique_ptr<Shape>>& shapes, Vec3 albedo)
{
	Vec3 color = Vec3(0,0,0);
	for (const std::unique_ptr<Light>& light : lights) {
		Vec3 lightDir = (light->position - point).normalize();
		double distanceCarrer = (light->position - point).dot(light->position - point);

		bool inShadow = false;
		Vec3 departure = point + normal * 0.001f;
		Rayon shadowRay = Rayon(departure, lightDir);
		for(const auto& shape : shapes) {
			std::optional<double> shadowIntersection = shape->intersect(shadowRay);
			if (shadowIntersection.has_value() && shadowIntersection.value() > 0.001f) {
				inShadow = true;
				break;
			}
		}

		if (!inShadow) {
			double diff = std::max(normal.dot(lightDir), 0.0);
			Vec3 contribution = (light->color * light->intensity / distanceCarrer) * diff * albedo;
			color = color + contribution;
		}
	}
	return color;
}*/

int main()
{
	Vec3 origin = Vec3(0, 0, -50);
	Camera camera = Camera(origin, 300, WIDTH, HEIGHT, 0, 150);
	Scene scene = Scene(camera);
	scene.addSphere(Sphere(10, Vec3(25, 0, 18), Vec3(1, 0, 0)));
	scene.addSphere(Sphere(15, Vec3(-45, 0, 20), Vec3(0, 1, 0)));
	scene.addSphere(Sphere(5, Vec3(12, 0, 15), Vec3(0, 0, 1)));
	scene.addPlane(Plane(Vec3(0, 50, 0), Vec3(0, 1, 0), Vec3(1, 1, 1)));
	scene.addPlane(Plane(Vec3(0, -50, 0), Vec3(0, -1, 0), Vec3(1, 1, 1)));
	scene.addPlane(Plane(Vec3(-50, 0, 0), Vec3(-1, 0, 0), Vec3(1, 0, 0)));
	scene.addPlane(Plane(Vec3(50, 0, 0), Vec3(1, 0, 0), Vec3(0, 0, 1)));
	scene.addPlane(Plane(Vec3(0, 0, 30), Vec3(0, 0, -1), Vec3(0, 1, 1)));
	scene.addLight(Light(Vec3(0, 0, -20), Vec3(1, 1, 0), 0.3));

	scene.generateDepthmap();
	scene.generateColormap();
	scene.generateNormalmap();
	scene.generateLightmap();

	std::vector<Vec3> pixels = scene.combineMaps();
	std::vector<Color> colorPixels = tonemap(pixels);


	writePPM("output.ppm", colorPixels, WIDTH, HEIGHT);
	system("start \"\" \"gimp-3.0.exe\" \"output.ppm\"");
}

// Exécuter le programme : Ctrl+F5 ou menu Déboguer > Exécuter sans débogage
// Déboguer le programme : F5 ou menu Déboguer > Démarrer le débogage

// Astuces pour bien démarrer : 
//   1. Utilisez la fenêtre Explorateur de solutions pour ajouter des fichiers et les gérer.
//   2. Utilisez la fenêtre Team Explorer pour vous connecter au contrôle de code source.
//   3. Utilisez la fenêtre Sortie pour voir la sortie de la génération et d'autres messages.
//   4. Utilisez la fenêtre Liste d'erreurs pour voir les erreurs.
//   5. Accédez à Projet > Ajouter un nouvel élément pour créer des fichiers de code, ou à Projet > Ajouter un élément existant pour ajouter des fichiers de code existants au projet.
//   6. Pour rouvrir ce projet plus tard, accédez à Fichier > Ouvrir > Projet et sélectionnez le fichier .sln.