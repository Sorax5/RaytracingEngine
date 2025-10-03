#include "Math.cpp"
#include "Image.h"
#include "Shape.h"
#include <vector>

#include <iostream>

#define WIDTH 800
#define HEIGHT 800

std::vector<double> getAllIntersections(const std::vector<std::unique_ptr<Shape>>& shapes, const Rayon& ray)
{
	std::vector<double> intersections = std::vector<double>(shapes.size());
	for (int i = 0; i < shapes.size(); i++) {
		const std::unique_ptr<Shape>& shape = shapes[i];
		std::optional<double> intersection = shape->intersect(ray);
		if (intersection.has_value()) {
			intersections[i] = intersection.value();
		}
		else {
			intersections[i] = -1;
		}
	}
	return intersections;
}

double getClosestIntersection(const std::vector<double>& intersections)
{
	double closestIntersection = INFINITY;
	int closestIndex = -1;
	for(int i = 0; i < intersections.size(); i++)
	{
		if (intersections[i] <= 0.001f)
		{
			continue;
		}
		if (intersections[i] < closestIntersection) {
			closestIntersection = intersections[i];
			closestIndex = i;
		}
	}

	return closestIndex;
}

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

Vec3 renderColor(const double& distance, const Shape& shape, const Camera& camera)
{
	double depth = 1 - (distance - camera.nearPlaneDistance) / (camera.farPlaneDistance - camera.nearPlaneDistance);
	return shape.getColor() * depth;
}

int main()
{
	Vec3 origin = Vec3(0, 0, -50);
	Camera camera = Camera(origin, 400, WIDTH, HEIGHT, 0, 150);

	std::vector<Vec3> pixels = std::vector<Vec3>(WIDTH * HEIGHT);

	std::vector<std::unique_ptr<Shape>> shapePointers = std::vector<std::unique_ptr<Shape>>();
	shapePointers.push_back(std::make_unique<Sphere>(Vec3(25, 0, 18), 10, Vec3(1, 0, 0)));
	shapePointers.push_back(std::make_unique<Sphere>(Vec3(-45, 0, 20), 15, Vec3(0, 1, 0)));
	shapePointers.push_back(std::make_unique<Sphere>(Vec3(12, 0, 15), 5, Vec3(0, 0, 1)));

	shapePointers.push_back(std::make_unique<Plane>(Vec3(0, 50, 0), Vec3(0, 1, 0), Vec3(1, 1, 1)));
	shapePointers.push_back(std::make_unique<Plane>(Vec3(0, -50, 0), Vec3(0, -1, 0), Vec3(1, 1, 1)));
	shapePointers.push_back(std::make_unique<Plane>(Vec3(-50, 0, 0), Vec3(-1, 0, 0), Vec3(1, 0, 0)));
	shapePointers.push_back(std::make_unique<Plane>(Vec3(50, 0, 0), Vec3(1, 0, 0), Vec3(0, 0, 1)));
	shapePointers.push_back(std::make_unique<Plane>(Vec3(0, 0, 30), Vec3(0, 0, -1), Vec3(0, 1, 1)));


	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			Rayon ray = camera.getRay(x, y);

			std::vector<double> intersections = getAllIntersections(shapePointers, ray);
			int nearestShapeIndex = getClosestIntersection(intersections);

			Vec3 color = Vec3(0, 0, 0);
			if (nearestShapeIndex != -1) {
				double distance = intersections[nearestShapeIndex];
				std::unique_ptr<Shape>& shape = shapePointers[nearestShapeIndex];
				color = renderColor(distance, *shape, camera);
			}
			
			pixels[y * WIDTH + x] = color;
		}
	}

	std::vector<Color> colorPixels = tonemap(pixels);

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