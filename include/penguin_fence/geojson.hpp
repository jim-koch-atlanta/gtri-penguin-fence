#pragma once

#include <string>

#include "geofence.hpp"
#include "mission.hpp"
#include "types.hpp"

namespace penguin_fence {

std::string toGeoJson(const MissionData& inputs, const GeofenceAsLatLonPoints& fence);

};