#include <gtest/gtest.h>

#include <vector>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/projection.hpp"
#include "penguin_fence/types.hpp"

namespace {

using penguin_fence::calculateCentroid;
using penguin_fence::LatLon;
using penguin_fence::MissionData;
using penguin_fence::Point2D;
using penguin_fence::Projection;

}  // namespace

// The AEQD center must project to the origin.
TEST(Projection, CenterMapsToOrigin) {
    LatLon center{.lat = -89.9, .lon = 12.0};
    Projection proj(center);
    Point2D m = proj.latLonToEastingNorthing(center);
    EXPECT_NEAR(m.easting, 0.0, 1e-6);
    EXPECT_NEAR(m.northing, 0.0, 1e-6);
}

// Forward then inverse returns the original -- the axis-order round-trip, in the
// near-pole regime where the mission actually lives.
TEST(Projection, RoundTripsNearPole) {
    Projection proj(LatLon{.lat = -89.9, .lon = 0.0});
    for (LatLon p : {LatLon{-89.85, 30.0}, LatLon{-89.95, -120.0},
                     LatLon{-89.80, 179.0}, LatLon{-89.99, -1.0}}) {
        Point2D m = proj.latLonToEastingNorthing(p);
        LatLon back = proj.eastingNorthingToLatLon(m);
        EXPECT_NEAR(back.lat, p.lat, 1e-9) << "at lon=" << p.lon;
        EXPECT_NEAR(back.lon, p.lon, 1e-9) << "at lon=" << p.lon;
    }
}

// Axis order: EAST of center -> +easting (x); NORTH of center -> +northing (y).
// This is what a lon/lat swap would fail. Centered on the equator so east/north
// align cleanly with lon/lat.
TEST(Projection, EastIsEastingNorthIsNorthing) {
    Projection proj(LatLon{.lat = 0.0, .lon = 0.0});

    Point2D east = proj.latLonToEastingNorthing(LatLon{.lat = 0.0, .lon = 0.01});
    EXPECT_GT(east.easting, 1000.0);        // ~1.1 km east
    EXPECT_NEAR(east.northing, 0.0, 1.0);

    Point2D north = proj.latLonToEastingNorthing(LatLon{.lat = 0.01, .lon = 0.0});
    EXPECT_GT(north.northing, 1000.0);      // ~1.1 km north
    EXPECT_NEAR(north.easting, 0.0, 1.0);
}

// Output is meters, not degrees: 1 degree of longitude at the equator ~ 111 km.
TEST(Projection, OutputIsInMeters) {
    Projection proj(LatLon{.lat = 0.0, .lon = 0.0});
    Point2D m = proj.latLonToEastingNorthing(LatLon{.lat = 0.0, .lon = 1.0});
    EXPECT_NEAR(m.easting, 111319.0, 500.0);
    EXPECT_NEAR(m.northing, 0.0, 1.0);
}

// Whole pipeline so far: mission -> centroid -> projection -> forward every
// vertex -> inverse -> back to the original.
TEST(Projection, ProjectsWholeMissionAndRoundTrips) {
    MissionData mission;
    mission.launchPoint = {-89.987080, -90.540186};
    mission.ingressRoute = {{-89.987080, -90.540186},
                            {-89.992746, -21.201396},
                            {-89.987957, 88.611099}};
    mission.regionOfInterest = {{-89.980833, 107.826869}, {-89.981868, 69.423914},
                                {-89.992081, 57.647665}, {-89.990410, 120.581590},
                                {-89.980833, 107.826869}};

    Projection proj(calculateCentroid(mission));

    std::vector<LatLon> all{mission.launchPoint};
    all.insert(all.end(), mission.ingressRoute.begin(), mission.ingressRoute.end());
    all.insert(all.end(), mission.regionOfInterest.begin(), mission.regionOfInterest.end());

    for (const LatLon& p : all) {
        Point2D projected = proj.latLonToEastingNorthing(p);
        LatLon back = proj.eastingNorthingToLatLon(projected);
        EXPECT_NEAR(back.lat, p.lat, 1e-7);
        EXPECT_NEAR(back.lon, p.lon, 1e-7);
    }
}
