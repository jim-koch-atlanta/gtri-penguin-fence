#include "penguin_fence/version.hpp"

#include <proj.h>
#include <geos_c.h>
#include <gdal/gdal.h>

namespace penguin_fence {

std::string version() {
    return "0.1.0";
}

std::string proj_version() {
    return proj_info().version;
}

std::string geos_version() {
    return GEOSversion();
}

std::string gdal_version() {
    const char* versionNum = GDALVersionInfo("VERSION_NUM");
    return versionNum;
}
}  // namespace penguin_fence
