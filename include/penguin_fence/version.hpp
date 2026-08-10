#pragma once

#include <string>

namespace penguin_fence {

// Returns the project version as a semantic-version string.
std::string version();

std::string proj_version();

std::string geos_version();

std::string gdal_version();

}  // namespace penguin_fence
