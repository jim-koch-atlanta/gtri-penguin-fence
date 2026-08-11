#include <gtest/gtest.h>

#include <vector>

#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"

namespace {

using penguin_fence::Geofence;
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

// Smoke test: building all three buffers for the spec mission must not throw.
// The real value is under the sanitizer gate -- it exercises the whole GEOS
// ownership dance (coordseq -> geometry -> buffer, all freed) and ASan/LSan
// proves nothing leaks or double-frees. (The boundary spot-check test with
// assertions on the buffers comes next, once the geofence exposes a result.)
TEST(Geofence, GeneratesSpecMissionWithoutThrowing) {
    Geofence geofence;
    EXPECT_NO_THROW(geofence.Generate(specMission()));
}
