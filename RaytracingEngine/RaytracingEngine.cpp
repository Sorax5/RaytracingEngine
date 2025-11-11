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

Vec3 clampVec3(const Vec3& v, double minVal = 0.0, double maxVal = 1.0) {
	return Vec3(
		std::min(maxVal, std::max(minVal, v.x)),
		std::min(maxVal, std::max(minVal, v.y)),
		std::min(maxVal, std::max(minVal, v.z))
	);
}

Vec3 uncharted2_tonemap_partial(Vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

Vec3 aces_approx(Vec3 v)
{
	v *= 0.6f;
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clampVec3((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
}

double luminance(const Vec3& color)
{
	Vec3 luminanceWeights = Vec3(0.2126, 0.7152, 0.0722);
	return color.dot(luminanceWeights);
}

Vec3 change_luminance(Vec3 c_in, double l_out)
{
	double l_in = luminance(c_in);
	return c_in * (l_out / l_in);
}


Color toColor(const Vec3& pixel)
{
    Vec3 clamped = clampVec3(pixel);
    return Color(
        static_cast<uint8_t>(clamped.x * 255.0),
        static_cast<uint8_t>(clamped.y * 255.0),
        static_cast<uint8_t>(clamped.z * 255.0)
    );
}

Vec3 simple(const Vec3& color)
{
	Vec3 mapped = Vec3(
		std::min(1.0, std::max(0.0, color.x)),
		std::min(1.0, std::max(0.0, color.y)),
		std::min(1.0, std::max(0.0, color.z))
	);
	return mapped;
}

Vec3 reinhardSimple(const Vec3& color) {
	return color / (color + 1);
}

Vec3 reinhardExtended(const Vec3 color, double max_white) {
	double white_sq = max_white * max_white;
	Vec3 numerator = color * ((color / Vec3(white_sq, white_sq, white_sq)) + 1);
	return numerator / (color + 1);
}

Vec3 reinhardExtendedLuminance(const Vec3& color, double maxWhite) {
	double L_old = luminance(color);
	double numerator = L_old * (1 + (L_old / (maxWhite * maxWhite)));
	double l_new = numerator / (1 + L_old);
	return change_luminance(color, l_new);
}

Vec3 reinhardJodie(const Vec3& color, double a = 0.18) {
	double L = luminance(color);
	double L_mapped = (a / std::log(2 + std::pow((L / 0.85), 1.7))) * std::log(1 + L);
	return change_luminance(color, L_mapped);
}

Vec3 uncharted2(const Vec3& color) {
	double exposureBias = 2.0f;
	Vec3 curr = uncharted2_tonemap_partial(color * exposureBias);

	Vec3 W = Vec3(11.2, 11.2, 11.2);
	Vec3 whiteScale = Vec3(1,1,1) / uncharted2_tonemap_partial(W);
	return curr * whiteScale;
}

std::vector<Color> tonemap(const std::vector<Vec3>& pixels)
{
	std::vector<Color> colorPixels = std::vector<Color>(pixels.size());
	for (size_t i = 0; i < pixels.size(); i++) {
		Vec3 current = pixels[i];
		Vec3 mapped = aces_approx(current);
		colorPixels[i] = toColor(mapped);
	}
	return colorPixels;
}

std::vector<std::vector<Color>> tonemapAll(const std::vector<Vec3> pixels)
{
	std::vector<std::vector<Color>> allTonemapped;
	std::vector<Color> simpleMapped = std::vector<Color>(pixels.size());
	std::vector<Color> reinhardSimpleMapped = std::vector<Color>(pixels.size());
	std::vector<Color> reinhardExtendedMapped = std::vector<Color>(pixels.size());
	std::vector<Color> reinhardExtendedLuminanceMapped = std::vector<Color>(pixels.size());
	std::vector<Color> reinhardJodieMapped = std::vector<Color>(pixels.size());
	std::vector<Color> uncharted2Mapped = std::vector<Color>(pixels.size());
	std::vector<Color> acesMapped = std::vector<Color>(pixels.size());

	for (size_t i = 0; i < pixels.size(); i++) {
		Vec3 current = pixels[i];
		Vec3 simpleColor = simple(current);
		simpleMapped[i] = toColor(simpleColor);
		Vec3 reinhardSimpleColor = reinhardSimple(current);
		reinhardSimpleMapped[i] = toColor(reinhardSimpleColor);
		Vec3 reinhardExtendedColor = reinhardExtended(current, 5.0);
		reinhardExtendedMapped[i] = toColor(reinhardExtendedColor);
		Vec3 reinhardExtendedLuminanceColor = reinhardExtendedLuminance(current, 5.0);
		reinhardExtendedLuminanceMapped[i] = toColor(reinhardExtendedLuminanceColor);
		Vec3 reinhardJodieColor = reinhardJodie(current);
		reinhardJodieMapped[i] = toColor(reinhardJodieColor);
		Vec3 uncharted2Color = uncharted2(current);
		uncharted2Mapped[i] = toColor(uncharted2Color);
		Vec3 acesColor = aces_approx(current);
		acesMapped[i] = toColor(acesColor);
	}

	allTonemapped.push_back(simpleMapped);
	allTonemapped.push_back(reinhardSimpleMapped);
	allTonemapped.push_back(reinhardExtendedMapped);
	allTonemapped.push_back(reinhardExtendedLuminanceMapped);
	allTonemapped.push_back(reinhardJodieMapped);
	allTonemapped.push_back(uncharted2Mapped);
	allTonemapped.push_back(acesMapped);

	return allTonemapped;
}

int main()
{
	int n_threads = omp_get_max_threads();
	std::cout << "Nombre de threads par défaut : " << n_threads << "\n";

	Vec3 origin = Vec3(0, 0, -10);
	Camera camera = Camera(origin, 500, WIDTH, HEIGHT, 0, 200);
	Scene scene = Scene(camera);

	Sphere sphere1(3, Vec3(-4, 0, 12), Vec3(1, 0, 0));
	Sphere sphere2(3, Vec3(4, 0, 15), Vec3(0, 0, 1));

	scene.addSphere(sphere1);
	scene.addSphere(sphere2);

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

        Plane plane(dir * -distance, dir, col);
        scene.addPlane(plane);
	}

	//scene.addLight(Light(Vec3(10, 10, 5), Vec3(0, 1, 1), 50));
	//scene.addLight(Light(Vec3(-10, -10, 5), Vec3(1, 0, 1), 50));

	Light light1(Vec3(0, 0, 0), Vec3(1, 1, 1), 100);
	Light light2(Vec3(0, 0, 8), Vec3(1, 1, 1), 75);
	scene.addLight(light1);
	scene.addLight(light2);

	// record time of generation

	auto gen_start = std::chrono::high_resolution_clock::now();
	std::vector<Vec3> pixels = scene.GenerateImage();
	auto gen_end = std::chrono::high_resolution_clock::now();

	auto gen_ms = std::chrono::duration_cast<std::chrono::milliseconds>(gen_end - gen_start).count();
	double gen_s = static_cast<double>(gen_ms) / 1000.0;

	std::cout << "Temps de génération de l'image : " << gen_ms << " ms (" << gen_s << " s)\n";

	std::vector<Color> colorPixels = tonemap(pixels);

	std::vector<std::string> tonemapNames = {
		"simple",
		"reinhard_simple",
		"reinhard_extended",
		"reinhard_extended_luminance",
		"reinhard_jodie",
		"uncharted2",
		"aces"
	};

	std::vector<std::vector<Color>> allTonemapped = tonemapAll(pixels);
	for (size_t i = 0; i < allTonemapped.size(); i++) {
		std::string tmFilename = tonemapNames[i];
		writePPM(tmFilename + ".ppm", allTonemapped[i], WIDTH, HEIGHT);
		std::string command = "ffmpeg -y -f image2 -i \"" + tmFilename + ".ppm\" \"" + tmFilename + ".png\"";
		int rc = std::system(command.c_str());
		if (rc == 0 && std::filesystem::exists(tmFilename + ".png")) {
			std::cout << "Conversion PPM -> PNG réussie :" << tmFilename << ".png\n";
			std::string deleteCommand = "del \"" + tmFilename + ".ppm\"";
			std::system(deleteCommand.c_str());
		}
		else {
			std::cerr << "Conversion PPM -> PNG échouée (code: " << rc << "). Vérifier que ffmpeg est installé et dans le PATH.\n";
		}
	}


	/*writePPM(filename + ".ppm", colorPixels, WIDTH, HEIGHT);

	std::string command = "ffmpeg -y -f image2 -i \"" + filename + ".ppm\" \"" + filename + ".png\"";
	int rc = std::system(command.c_str());
	if (rc == 0 && std::filesystem::exists(filename + ".png")) {
		std::cout << "Conversion PPM -> PNG réussie :" << filename << ".png\n";
		std::string deleteCommand = "del \"" + filename + ".ppm\"";
		std::system(deleteCommand.c_str());
	}
	else {
		std::cerr << "Conversion PPM -> PNG échouée (code: " << rc << "). Vérifier que ffmpeg est installé et dans le PATH.\n";
	}*/

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