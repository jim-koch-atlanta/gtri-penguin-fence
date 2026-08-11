#include <cstdio>
#include <iostream>
#include <string_view>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/parser.hpp"
#include "penguin_fence/projection.hpp"
#include "penguin_fence/version.hpp"

using namespace penguin_fence;

void testGeosIntegration() {
    constexpr std::string_view missionJson = R"({
    "launchPoint": "89.987080 S 90.540186 W",
    "ingressRoute": [
        "89.987080 S 90.540186 W",
        "89.992746 S 21.201396 W",
        "89.987957 S 88.611099 E"
    ],
    "regionOfInterest": [
        "89.980833 S 107.826869 E",
        "89.981868 S 69.423914 E",
        "89.992081 S 57.647665 E",
        "89.990410 S 120.581590 E",
        "89.980833 S 107.826869 E"
    ]
    })";

    auto r = parseMission(missionJson);
    if (!r.ok()) { std::printf("parse failed: %s\n", r.error().c_str()); return; }
    const MissionData& m = r.value();

    LatLon c = calculateCentroid(m);
    Projection proj(c);
    std::printf("centroid: lat=%.6f lon=%.6f\n\n", c.lat, c.lon);

    auto show = [&](const char* label, const LatLon& p) {
        Point2D e = proj.latLonToEastingNorthing(p);
        std::printf("%-8s lat=%.6f lon=%11.6f  ->  E=%9.1f m  N=%9.1f m\n",
                    label, p.lat, p.lon, e.easting, e.northing);
    };
    show("launch", m.launchPoint);
    for (auto& p : m.ingressRoute) show("route", p);
    for (auto& p : m.regionOfInterest) show("roi", p);
}

int main() {
    testGeosIntegration();
    return 0;
}
