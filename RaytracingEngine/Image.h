#pragma once

#include <string>
#include <vector>
#include "Math.h"

void writePPM(const std::string& filename, const std::vector<Color>& pixels, const size_t width, const size_t height);