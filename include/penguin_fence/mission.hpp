#pragma once

#include "latlon.hpp"

#include <vector>

namespace penguin_fence {

/**
 * Mission data, which includes a launch point, .
 */
struct MissionData {
    LatLon launchPoint;
    std::vector<LatLon> ingressRoute;
    std::vector<LatLon> regionOfInterest; // last == first
};

}
