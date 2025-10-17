#include "Math.h"
#include "Image.h"
#include "Light.h"
#include "Shape.h"
#include "Scene.h"
#include <vector>
#include <filesystem>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 1000
#define HEIGHT 1000

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
	Vec3 origin = Vec3(0, 0, -10);
	Camera camera = Camera(origin, 500, WIDTH, HEIGHT, 0, 250);
	Scene scene = Scene(camera);
	/*scene.addSphere(Sphere(10, Vec3(25, 0, 18), Vec3(1, 0, 0)));
	scene.addSphere(Sphere(15, Vec3(-45, 0, 20), Vec3(0, 1, 0)));
	scene.addSphere(Sphere(5, Vec3(12, 0, 15), Vec3(0, 0, 1)));*/

	scene.addSphere(Sphere(4, Vec3(8, 0, 10), Vec3(1, 1, 1)));
	scene.addSphere(Sphere(4, Vec3(-8, 0, 7), Vec3(1, 1, 1)));
	double distance = 15;

	std::vector<Vec3> normalDirections = {
		Vec3(0, 0, -1),
		Vec3(1, 0, 0),
		Vec3(-1, 0, 0),
		Vec3(0, 1, 0),
		Vec3(0, -1, 0)
	};

	std::vector<Vec3> planeColors = {
		Vec3(1, 1, 1),
		Vec3(0, 1, 0),
		Vec3(0, 0, 1),
		Vec3(1, 1, 1),
		Vec3(1, 1, 1)
	};

	for (int i = 0; i < normalDirections.size(); i++)
	{
		Vec3 dir = normalDirections[i];
		Vec3 col = planeColors[i];
		scene.addPlane(Plane(dir * -distance, dir, col));
	}

	scene.addLight(Light(Vec3(10, 10, 5), Vec3(0, 1, 1), 50));
	scene.addLight(Light(Vec3(-10, -10, 5), Vec3(1, 0, 1), 50));
	scene.addLight(Light(Vec3(0, 0, 0), Vec3(1, 1, 1), 30));

	scene.generateDepthmap();
	scene.generateColormap();
	scene.generateNormalmap();
	scene.generateLightmap();

	std::vector<Vec3> pixels = scene.combineMaps();
	std::vector<Color> colorPixels = tonemap(pixels);


	writePPM("output.ppm", colorPixels, WIDTH, HEIGHT);	
	//system("start \"\" \"gimp-3.0.exe\" \"output.ppm\"");

	int rc = std::system("ffmpeg -y -f image2 -i \"output.ppm\" \"output.png\"");
	if (rc == 0 && std::filesystem::exists("output.png")) {
		std::cout << "Conversion PPM -> PNG réussie : output.png\n";
		std::system("start \"\" \"output.png\"");
	}
	else {
		std::cerr << "Conversion PPM -> PNG échouée (code: " << rc << "). Vérifier que ffmpeg est installé et dans le PATH.\n";
	}

	return 0;
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