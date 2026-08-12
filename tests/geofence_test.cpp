#include <gtest/gtest.h>

#include <vector>

#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

namespace {

using penguin_fence::Geofence;
using penguin_fence::LatLon;
using penguin_fence::MissionData;

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
