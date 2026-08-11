#pragma once

#include <concepts>
#include <numbers>
#include <vector>

#include "mission.hpp"
#include "types.hpp"

namespace penguin_fence {

template <std::floating_point T>
constexpr T degrees_to_radians(T deg) {
    return deg * (std::numbers::pi_v<T> / static_cast<T>(180.0));
}

template <std::floating_point T>
constexpr T radians_to_degrees(T rad) {
    return rad * (static_cast<T>(180.0) / std::numbers::pi_v<T>);
}

[[nodiscard]] Point3D toPoint3D(LatLon latLon);

[[nodiscard]] LatLon calculateCentroid(const std::vector<LatLon>& points);

[[nodiscard]] LatLon calculateCentroid(const MissionData& mission);

}  // namespace penguin_fence
