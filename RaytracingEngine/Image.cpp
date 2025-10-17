#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <cmath>
#include "Math.h"

// write vector<Vec3> to a PPM file
void writePPM(const std::string& filename, const std::vector<Color>& pixels, int width, int height)
{
	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	if (!ofs) {
		throw std::runtime_error("Could not open file for writing");
	}

	ofs << "P6\n" << width << " " << height << "\n255\n";
	for (const auto& pixel : pixels) {
		ofs << static_cast<char>(std::min(255, std::max(0, static_cast<int>(pixel.r))))
			<< static_cast<char>(std::min(255, std::max(0, static_cast<int>(pixel.g))))
			<< static_cast<char>(std::min(255, std::max(0, static_cast<int>(pixel.b))));
	}

	ofs.close();
	if (!ofs) {
		throw std::runtime_error("Error occurred while writing to file");
	}

	std::cout << "Image written to " << filename << std::endl;
}
