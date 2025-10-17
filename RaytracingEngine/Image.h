#pragma once

#include <string>
#include <vector>
#include "Math.h"

void writePPM(const std::string& filename, const std::vector<Color>& pixels, int width, int height);