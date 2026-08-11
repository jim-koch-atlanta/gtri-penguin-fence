#include <cmath>
#include <concepts>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

namespace penguin_fence {

[[nodiscard]]
Point3D toPoint3D(LatLon latlon) {
    double latRadians = degrees_to_radians(latlon.lat);
    double lonRadians = degrees_to_radians(latlon.lon);
    auto x = std::cos(latRadians) * std::cos(lonRadians);
    auto y = std::cos(latRadians) * std::sin(lonRadians);
    auto z = std::sin(latRadians);
    return {x, y, z};
}

[[nodiscard]]
LatLon calculateCentroid(const std::vector<LatLon>& latLons) {
    // Step 1. Convert each lat/lon to a unit vector, and sum them up.
    double x_sum = 0.0;
    double y_sum = 0.0;
    double z_sum = 0.0;
    
    for (LatLon latLon : latLons) {
        Point3D p = toPoint3D(latLon);
        x_sum += p.x;
        y_sum += p.y;
        z_sum += p.z;
    }

    // Step 2. Average the vectors, component-wise.
    double x_avg = x_sum / latLons.size();
    double y_avg = y_sum / latLons.size();
    double z_avg = z_sum / latLons.size();

    // Step 3. Assert norm > epsilon ???
    constexpr double kMinCentroidNorm = 1e-9;
    double norm = std::sqrt(x_avg * x_avg + y_avg * y_avg + z_avg * z_avg);
    if (norm < kMinCentroidNorm) {
        // TODO: Switch from exception to graceful failure.
        throw std::invalid_argument(
            "spherical centroid undefined: input points are balanced "
            "symmetrically about the sphere's center (mean vector ~0)");
    }

    // Step 4. Convert the mean vector back to lat / lon
    //   Use atan2() instead of atan(), so it will resolve the quadrant from the
    //   signs of the arguments.
    double lat = std::atan2( z_avg, std::sqrt(x_avg * x_avg + y_avg * y_avg) );
    double lon = std::atan2( y_avg, x_avg );
    double latDegrees = radians_to_degrees(lat);
    double lonDegrees = radians_to_degrees(lon);

    return { latDegrees, lonDegrees };
}

[[nodiscard]]
LatLon calculateCentroid(const MissionData& mission) {
    std::vector<LatLon> latLons;
    latLons.push_back(mission.launchPoint);
    for (auto p : mission.ingressRoute) {
        latLons.push_back(p);
    }
    for (auto p : mission.regionOfInterest) {
        latLons.push_back(p);
    }

    return calculateCentroid(latLons);
}

}