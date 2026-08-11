#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

namespace {

using penguin_fence::calculateCentroid;
using penguin_fence::degrees_to_radians;
using penguin_fence::LatLon;
using penguin_fence::MissionData;
using penguin_fence::Point3D;
using penguin_fence::radians_to_degrees;
using penguin_fence::toPoint3D;

constexpr double kDegTol = 1e-6;   // degrees

}  // namespace

// --- angle helpers ---------------------------------------------------------
TEST(Angles, DegreesToRadiansKnownValues) {
    EXPECT_NEAR(degrees_to_radians(180.0), std::numbers::pi, 1e-12);
    EXPECT_NEAR(degrees_to_radians(90.0), std::numbers::pi / 2, 1e-12);
}
TEST(Angles, RoundTrips) {
    for (double d : {-179.5, -90.0, 0.0, 45.0, 179.5}) {
        EXPECT_NEAR(radians_to_degrees(degrees_to_radians(d)), d, 1e-12);
    }
}

// --- toPoint3D: lat/lon -> unit vector -------------------------------------
TEST(ToPoint3D, CardinalDirections) {
    auto near = [](Point3D p, double x, double y, double z) {
        EXPECT_NEAR(p.x, x, 1e-12);
        EXPECT_NEAR(p.y, y, 1e-12);
        EXPECT_NEAR(p.z, z, 1e-12);
    };
    near(toPoint3D({0.0, 0.0}),   1.0, 0.0, 0.0);    // equator, prime meridian -> +x
    near(toPoint3D({0.0, 90.0}),  0.0, 1.0, 0.0);    // equator, 90E            -> +y
    near(toPoint3D({90.0, 0.0}),  0.0, 0.0, 1.0);    // north pole              -> +z
    near(toPoint3D({-90.0, 0.0}), 0.0, 0.0, -1.0);   // south pole              -> -z
}

// --- calculateCentroid -----------------------------------------------------
TEST(Centroid, SinglePointIsIdentity) {
    LatLon c = calculateCentroid(std::vector<LatLon>{{12.34, -56.78}});
    EXPECT_NEAR(c.lat, 12.34, kDegTol);
    EXPECT_NEAR(c.lon, -56.78, kDegTol);
}

// THE headline of step 2: averaging across the antimeridian must NOT collapse
// to the prime meridian (the bug that naive lat/lon averaging produces).
TEST(Centroid, AntimeridianDoesNotAverageToZero) {
    LatLon c = calculateCentroid(std::vector<LatLon>{{0.0, 179.0}, {0.0, -179.0}});
    EXPECT_NEAR(c.lat, 0.0, kDegTol);
    EXPECT_GT(std::abs(c.lon), 179.0);   // ~+/-180, emphatically not ~0
}

TEST(Centroid, SymmetricAboutEquator) {
    // Mirrored across the equator at the same longitude -> latitude 0.
    LatLon c = calculateCentroid(std::vector<LatLon>{{10.0, 20.0}, {-10.0, 20.0}});
    EXPECT_NEAR(c.lat, 0.0, kDegTol);
    EXPECT_NEAR(c.lon, 20.0, kDegTol);
}

TEST(Centroid, PoleClusterStaysAtPole) {
    // Longitudes spread around the south pole; z-bar dominates -> lat ~ -90.
    // (Longitude is meaningless at the pole, so we don't assert it.)
    LatLon c = calculateCentroid(std::vector<LatLon>{
        {-89.0, 0.0}, {-89.0, 90.0}, {-89.0, 180.0}, {-89.0, -90.0}});
    EXPECT_LT(c.lat, -88.9);
}

TEST(Centroid, MissionDelegatesToItsVertices) {
    MissionData m;
    m.launchPoint = {10.0, 20.0};
    m.ingressRoute = {{10.0, 20.0}, {11.0, 21.0}};
    m.regionOfInterest = {{9.0, 19.0}, {10.0, 20.0}};

    // The MissionData overload should equal the vector overload over the same
    // vertices, gathered in the same order (launch, route..., ROI...).
    const std::vector<LatLon> all{
        {10.0, 20.0}, {10.0, 20.0}, {11.0, 21.0}, {9.0, 19.0}, {10.0, 20.0}};

    LatLon viaMission = calculateCentroid(m);
    LatLon viaVector = calculateCentroid(all);
    EXPECT_NEAR(viaMission.lat, viaVector.lat, kDegTol);
    EXPECT_NEAR(viaMission.lon, viaVector.lon, kDegTol);
}

TEST(Centroid, CannotCalculateForGlobalRegion) {
    // Antipodal points -> mean vector ~0 -> centroid undefined -> throws.
    EXPECT_THROW((void)calculateCentroid(std::vector<LatLon>{{0.0, 0.0}, {0.0, 180.0}}),
                 std::invalid_argument);
}
