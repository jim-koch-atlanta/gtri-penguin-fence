#include <gtest/gtest.h>

#include <geodesic.h>   // PROJ's bundled GeographicLib geodesic C API

#include <algorithm>
#include <limits>
#include <vector>

#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

// V2 -- geodesic ground-truth probes (TECH_SPEC section 7).
//
// Each probe is placed by geodesic distance on the WGS84 ellipsoid (geod_direct),
// INDEPENDENT of our AEQD projection; its distance back to the target geometry is
// then re-measured (geod_inverse) as a ground-truth check BEFORE asserting
// containment. Containment is evaluated in PROJECTED space only -- via
// Geofence::contains, which projects the point and runs GEOSContains -- never on
// raw lon/lat. (The geodesic truth and the projected containment agree because
// AEQD distortion over the ~2 km mission is ~1e-8.)

namespace {

using penguin_fence::Geofence;
using penguin_fence::LatLon;
using penguin_fence::MissionData;

geod_geodesic wgs84() {
    geod_geodesic g;
    geod_init(&g, 6378137.0, 1.0 / 298.257223563);   // WGS84 semi-major axis, flattening
    return g;
}

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

// One mission + its geofence + the WGS84 geodesic, plus geodesic helpers so each
// test just names anchors, azimuths, and distances.
class V2Probe : public ::testing::Test {
  protected:
    const MissionData mission = specMission();
    Geofence fence{ mission };
    const geod_geodesic geod = wgs84();

    // Point `dist` m from `anchor` along `azimuth` (deg clockwise from north).
    LatLon along(LatLon anchor, double azimuth, double dist) const {
        double lat = 0.0, lon = 0.0;
        geod_direct(&geod, anchor.lat, anchor.lon, azimuth, dist, &lat, &lon, nullptr);
        return {.lat = lat, .lon = lon};
    }

    // Initial bearing (deg) from a toward b.
    double bearing(LatLon a, LatLon b) const {
        double s12 = 0.0, azi1 = 0.0;
        geod_inverse(&geod, a.lat, a.lon, b.lat, b.lon, &s12, &azi1, nullptr);
        return azi1;
    }

    // Midpoint of segment a->b and a perpendicular azimuth there (side = +1/-1).
    struct Perp { LatLon midpoint; double azimuth; };
    Perp midpointPerp(LatLon a, LatLon b, double side) const {
        double s12 = 0.0, azi1 = 0.0;
        geod_inverse(&geod, a.lat, a.lon, b.lat, b.lon, &s12, &azi1, nullptr);
        double lat = 0.0, lon = 0.0, aziAtMid = 0.0;
        geod_direct(&geod, a.lat, a.lon, azi1, s12 / 2.0, &lat, &lon, &aziAtMid);
        return {{.lat = lat, .lon = lon}, aziAtMid + side * 90.0};
    }

    // Geodesic distance (m) between two points -- ground truth for a point target.
    double distance(LatLon a, LatLon b) const {
        double s12 = 0.0;
        geod_inverse(&geod, a.lat, a.lon, b.lat, b.lon, &s12, nullptr, nullptr);
        return s12;
    }

    // Min geodesic distance from p to a polyline/ring, by densifying each segment
    // at ~1 m -- ground truth for a line/ring target.
    double distanceToLine(LatLon p, const std::vector<LatLon>& verts) const {
        double best = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i + 1 < verts.size(); ++i) {
            const double segLen = distance(verts[i], verts[i + 1]);
            const double azi = bearing(verts[i], verts[i + 1]);
            const int steps = std::max(1, static_cast<int>(segLen));   // ~1 m spacing
            for (int k = 0; k <= steps; ++k)
                best = std::min(best, distance(p, along(verts[i], azi, segLen * k / steps)));
        }
        return best;
    }
};

}  // namespace

// --- Launch buffer: 199 m inside / 201 m outside, aimed AWAY from the route ---
TEST_F(V2Probe, LaunchBuffer) {
    // Aim opposite the route's initial bearing so the 201 m probe clears every
    // component, not just the launch disc.
    const LatLon launch = mission.launchPoint;
    const double away = bearing(launch, mission.ingressRoute[1]) + 180.0;

    const LatLon inner = along(launch, away, 199.0);
    EXPECT_NEAR(distance(inner, launch), 199.0, 1.0);
    EXPECT_TRUE(fence.contains(inner));

    const LatLon outer = along(launch, away, 201.0);
    EXPECT_NEAR(distance(outer, launch), 201.0, 1.0);
    EXPECT_FALSE(fence.contains(outer));
}

// --- Route buffer: 99 m inside / 101 m outside, perpendicular to a mid-segment ---
TEST_F(V2Probe, RouteBuffer) {
    // Off the launch->route[1] segment midpoint; either side clears (the midpoint
    // is ~690 m from launch and far from the ROI).
    const auto [mid, perp] = midpointPerp(mission.ingressRoute[0], mission.ingressRoute[1], +1.0);

    const LatLon inner = along(mid, perp, 99.0);
    EXPECT_NEAR(distanceToLine(inner, mission.ingressRoute), 99.0, 1.0);
    EXPECT_TRUE(fence.contains(inner));

    const LatLon outer = along(mid, perp, 101.0);
    EXPECT_NEAR(distanceToLine(outer, mission.ingressRoute), 101.0, 1.0);
    EXPECT_FALSE(fence.contains(outer));
}

// --- ROI buffer: 249 m inside / 251 m outside, outward from an edge midpoint ---
TEST_F(V2Probe, RoiBuffer) {
    // Off an ROI edge midpoint. Here +90 is the OUTWARD side; -90 would point into
    // the polygon.
    const auto [mid, perp] = midpointPerp(mission.regionOfInterest[2], mission.regionOfInterest[3], +1.0);

    const LatLon inner = along(mid, perp, 249.0);
    EXPECT_NEAR(distanceToLine(inner, mission.regionOfInterest), 249.0, 1.0);
    EXPECT_TRUE(fence.contains(inner));

    const LatLon outer = along(mid, perp, 251.0);
    EXPECT_NEAR(distanceToLine(outer, mission.regionOfInterest), 251.0, 1.0);
    EXPECT_FALSE(fence.contains(outer));
}

// --- Distant probe: 5 km out, well beyond the fence's ~2.3 km extent ---
TEST_F(V2Probe, DistantPointOutsideFence) {
    const LatLon far = along(mission.launchPoint, 0.0, 5000.0);   // 5 km, any direction
    EXPECT_NEAR(distance(far, mission.launchPoint), 5000.0, 1.0);
    EXPECT_FALSE(fence.contains(far));
}
