#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/projection.hpp"
#include "penguin_fence/types.hpp"

namespace {

using penguin_fence::calculateCentroid;
using penguin_fence::Geofence;
using penguin_fence::GeofenceAsLatLonPoints;
using penguin_fence::LatLon;
using penguin_fence::MissionData;
using penguin_fence::Point2D;
using penguin_fence::Projection;

MissionData specMission() {
    MissionData m;
    m.launchPoint = {-89.987080, -90.540186};
    m.ingressRoute = {{-89.987080, -90.540186},
                      {-89.992746, -21.201396},
                      {-89.987957, 88.611099}};
    m.regionOfInterest = {{-89.980833, 107.826869}, {-89.981868, 69.423914},
                          {-89.992081, 57.647665}, {-89.990410, 120.581590},
                          {-89.980833, 107.826869}};
    return m;
}

}  // namespace

// Construction builds all three buffers + the union. Leak/ownership behavior is
// validated by the ASan/UBSan/LeakSan gate.
TEST(Geofence, ConstructsSpecMissionWithoutThrowing) {
    EXPECT_NO_THROW(Geofence{ specMission() });
}

// The union of the three buffers contains every mission vertex (each sits inside
// its own component buffer) and excludes points far outside the ~2 km mission.
TEST(Geofence, ContainsMissionVerticesAndExcludesFarPoints) {
    const MissionData mission = specMission();
    Geofence fence{ mission };

    EXPECT_TRUE(fence.contains(mission.launchPoint));
    for (const LatLon& p : mission.ingressRoute) EXPECT_TRUE(fence.contains(p));
    for (const LatLon& p : mission.regionOfInterest) EXPECT_TRUE(fence.contains(p));

    EXPECT_FALSE(fence.contains(LatLon{.lat = -88.0, .lon = 0.0}));   // ~110 km from the cluster
    EXPECT_FALSE(fence.contains(LatLon{.lat = 0.0, .lon = 0.0}));     // the equator
}

// asLatLonPoints() inverse-projects each component back to WGS84. The rings must
// come out closed and sitting at the mission (near the south pole) -- a botched
// inverse would smear them elsewhere on the globe.
TEST(Geofence, OutputComponentsAreClosedRingsNearThePole) {
    Geofence fence{ specMission() };
    GeofenceAsLatLonPoints out = fence.asLatLonPoints();

    auto checkRing = [](const std::vector<LatLon>& ring) {
        ASSERT_GE(ring.size(), 4u);
        EXPECT_NEAR(ring.front().lat, ring.back().lat, 1e-9);   // closed
        EXPECT_NEAR(ring.front().lon, ring.back().lon, 1e-9);
        for (const LatLon& p : ring) {
            EXPECT_LT(p.lat, -89.0);                            // near the south pole
            EXPECT_GE(p.lon, -180.0);
            EXPECT_LE(p.lon, 180.0);
        }
    };
    checkRing(out.launchPointFence);
    checkRing(out.ingressRouteFence);
    checkRing(out.regionOfInterestFence);
    checkRing(out.mergedFence);
}

// End-to-end round trip: re-project the WGS84 launch-buffer ring back to meters;
// every vertex must sit ~200 m from the launch point (the buffer radius). An
// axis-order or inverse-transform error would show up here as a wrong radius.
TEST(Geofence, OutputLaunchBufferIsA200mRing) {
    const MissionData mission = specMission();
    Geofence fence{ mission };
    GeofenceAsLatLonPoints out = fence.asLatLonPoints();

    Projection proj{ calculateCentroid(mission) };
    const Point2D center = proj.latLonToEastingNorthing(mission.launchPoint);
    for (const LatLon& p : out.launchPointFence) {
        const Point2D m = proj.latLonToEastingNorthing(p);
        const double d = std::hypot(m.easting - center.easting, m.northing - center.northing);
        EXPECT_NEAR(d, 200.0, 1.0);
    }
}
