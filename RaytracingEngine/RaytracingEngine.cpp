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
	for (int i = 0; i < static_cast<int>(pixels.size()); i++) {
		colorPixels[i] = Color(
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].x * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].y * 255.0))),
			static_cast<uint8_t>(std::min(255.0, std::max(0.0, pixels[i].z * 255.0)))
		);
	}
	return colorPixels;
}

int main()
{
	Vec3 origin = Vec3(0, 0, -50);
	Camera camera = Camera(origin, 300, WIDTH, HEIGHT, 0, 150);
	Scene scene = Scene(camera);
	scene.addSphere(Sphere(10, Vec3(25, 0, 18), Vec3(1, 0, 0)));
	scene.addSphere(Sphere(15, Vec3(-45, 0, 20), Vec3(0, 1, 0)));
	scene.addSphere(Sphere(5, Vec3(12, 0, 15), Vec3(0, 0, 1)));

	std::vector<Vec3> normalDirections = {
		Vec3(0, 0,1),
		Vec3(1, 0, 0),
		Vec3(-1, 0, 0),
		Vec3(0, 1, 0),
		Vec3(0, -1, 0)
	};

	for (auto& dir : normalDirections) {
		scene.addPlane(Plane(dir * 50, dir, Vec3((dir.x + 1) * 0.5f, (dir.y + 1) * 0.5f, (dir.z + 1) * 0.5f)));
	}

	scene.addLight(Light(Vec3(0, 0, 0), Vec3(1, 1, 1), 10));

	scene.generateDepthmap();
	scene.generateColormap();
	scene.generateNormalmap();
	scene.generateLightmap();

	// Utiliser la combinaison couleur * lumière pour afficher le rendu final
	std::vector<Vec3> pixels = scene.getLightMap();
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