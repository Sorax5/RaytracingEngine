#include "Math.cpp"
#include "Image.h"
#include <vector>

#include <iostream>

#define WIDTH 800
#define HEIGHT 800

int main()
{
	Vec3 origin = Vec3(0, 0, 0);
	Camera camera = Camera(origin, 100, WIDTH, HEIGHT, 20, 65);

	std::vector<Vec3> pixels = std::vector<Vec3>(WIDTH * HEIGHT);
	std::vector<Sphere> spheres = std::vector<Sphere>(3);

	Sphere sphere = Sphere(Vec3(25, 0, 55), 20, Vec3(1, 0, 0));
	spheres[0] = sphere;
	Sphere sphere2 = Sphere(Vec3(-10, 0, 80), 40, Vec3(0, 1, 0));
	spheres[1] = sphere2;
	Sphere sphere3 = Sphere(Vec3(12, 0, 45), 10, Vec3(0, 0, 1));
	spheres[2] = sphere3;

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			Rayon ray = camera.getRay(x, y);

			std::vector<double> intersections = std::vector<double>(spheres.size());
			for(int i = 0; i < spheres.size(); i++) {
				Sphere& currentSphere = spheres[i];
				std::optional<double> intersection = currentSphere.intersect(ray);
				if (intersection.has_value()) {
					intersections[i] = intersection.value();
				}
				else {
					intersections[i] = -1;
				}
			}


			double closestIntersection = INFINITY;
			int closestSphereIndex = -1;
			for (int i = 0; i < intersections.size(); i++) {
				double intersection = intersections[i];
				if (intersection <= 0.001f)
				{
					continue;
				}

				if (intersection < closestIntersection) {
					closestIntersection = intersection;
					closestSphereIndex = i;
				}
			}

			Vec3 color = Vec3(0, 0, 0);
			if (closestSphereIndex != -1) {
				double distance = intersections[closestSphereIndex];
				double brightness = (distance - camera.nearPlaneDistance) / (camera.farPlaneDistance - camera.nearPlaneDistance);
				brightness = 1.0 - brightness;

				Sphere& closestSphere = spheres[closestSphereIndex];
				color = closestSphere.color * brightness;
			}
			
			pixels[y * WIDTH + x] = color;
		}
	}

	std::vector<Color> colorPixels = std::vector<Color>(WIDTH * HEIGHT);
	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		colorPixels[i] = Color(
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].x * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].y * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].z * 255.0)))
		);
	}

	writePPM("output.ppm", colorPixels, WIDTH, HEIGHT);
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