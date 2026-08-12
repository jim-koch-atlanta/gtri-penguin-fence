#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "penguin_fence/geofence.hpp"
#include "penguin_fence/geojson.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

using json = nlohmann::json;

namespace {

using penguin_fence::Geofence;
using penguin_fence::LatLon;
using penguin_fence::MissionData;
using penguin_fence::toGeoJson;

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

// toGeoJson must emit a valid GeoJSON FeatureCollection: the input geometries
// (Point / LineString / Polygon) plus the buffer/fence polygons, with
// coordinates in [lon, lat] order and polygon rings closed.
TEST(GeoJson, EmitsValidFeatureCollection) {
    const MissionData mission = specMission();
    Geofence fence{ mission };
    const std::string out = toGeoJson(mission, fence.asLatLonPoints());

    json doc;
    ASSERT_NO_THROW(doc = json::parse(out)) << "toGeoJson did not return valid JSON";

    EXPECT_EQ(doc.at("type").get<std::string>(), "FeatureCollection");
    ASSERT_TRUE(doc.at("features").is_array());
    ASSERT_FALSE(doc.at("features").empty());

    // Walk features by geometry type (independent of your property scheme).
    std::map<std::string, int> geomTypes;
    const json* pointFeature = nullptr;
    const json* polygonFeature = nullptr;
    for (const json& feature : doc.at("features")) {
        EXPECT_EQ(feature.at("type").get<std::string>(), "Feature");
        const json& geom = feature.at("geometry");
        const std::string type = geom.at("type").get<std::string>();
        const std::string name = feature.at("properties").at("name").get<std::string>();
        geomTypes[type]++;
        ASSERT_TRUE(geom.at("coordinates").is_array());
        if (type == "Point" && !pointFeature) pointFeature = &feature;
        if (type == "Polygon" && name == "fence_region_of_interest" && !polygonFeature) polygonFeature = &feature;
    }

    // All three input geometry kinds are present.
    EXPECT_GE(geomTypes["Point"], 1);
    EXPECT_GE(geomTypes["LineString"], 1);
    EXPECT_GE(geomTypes["Polygon"], 1);

    // Axis order: the launch Point must be [lon, lat] -- longitude first.
    ASSERT_NE(pointFeature, nullptr);
    const json& coords = pointFeature->at("geometry").at("coordinates");
    ASSERT_EQ(coords.size(), 2u);
    EXPECT_NEAR(coords[0].get<double>(), mission.launchPoint.lon, 1e-9) << "longitude must come first";
    EXPECT_NEAR(coords[1].get<double>(), mission.launchPoint.lat, 1e-9);

    // A polygon's exterior ring must be closed (first coord == last coord).
    ASSERT_NE(polygonFeature, nullptr);
    const json& rings = polygonFeature->at("geometry").at("coordinates");   // [ ring, holes... ]
    ASSERT_TRUE(rings.is_array());
    ASSERT_FALSE(rings.empty());
    const json& ring = rings[0];
    ASSERT_GE(ring.size(), 4u);
    EXPECT_EQ(ring.front(), ring.back()) << "polygon ring must be closed";
}
