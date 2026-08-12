#include <string>

#include <nlohmann/json.hpp>

#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/types.hpp"

using json = nlohmann::json;

namespace penguin_fence {

json pointToGeoJson(const LatLon& point, std::string_view name, std::string_view role) {
    json resultObj = json::object();
    resultObj["type"] = "Feature";
    resultObj["properties"]["name"] = name;
    resultObj["properties"]["role"] = role;
    resultObj["geometry"]["type"] = "Point";
    resultObj["geometry"]["coordinates"] = {point.lon, point.lat};

    return resultObj;
}

json polygonToGeoJson(const std::vector<LatLon>& polygon, std::string_view name, std::string_view role, std::string_view type = "Polygon") {
    json coords = json::array();
    for (const LatLon& p : polygon) {
        coords.push_back({ p.lon, p.lat });
    }

    json resultObj = json::object();
    resultObj["type"] = "Feature";
    resultObj["properties"]["name"] = name;
    resultObj["properties"]["role"] = role;
    resultObj["geometry"]["type"] = type;
    // A Polygon's coordinates are an array of rings ([[[lon,lat],...]]); a
    // LineString's are a bare array of points ([[lon,lat],...]).
    resultObj["geometry"]["coordinates"] = (type == "Polygon") ? json::array({ coords }) : coords;

    return resultObj;
};

std::string toGeoJson(const MissionData& inputs, const GeofenceAsLatLonPoints& fence) {

    json resultObj = json::object();
    resultObj["type"] = "FeatureCollection";

    json featuresObj = json::array();
    featuresObj.push_back(pointToGeoJson(inputs.launchPoint, "launch_point", "input"));
    featuresObj.push_back(polygonToGeoJson(inputs.ingressRoute, "ingress_route", "input", "LineString"));
    featuresObj.push_back(polygonToGeoJson(inputs.regionOfInterest, "region_of_interest", "input"));

    featuresObj.push_back(polygonToGeoJson(fence.launchPointFence, "fence_launch_point", "fence"));
    featuresObj.push_back(polygonToGeoJson(fence.ingressRouteFence, "fence_ingress_route", "fence"));
    featuresObj.push_back(polygonToGeoJson(fence.regionOfInterestFence, "fence_region_of_interest", "input"));

    featuresObj.push_back(polygonToGeoJson(fence.mergedFence, "fence", "fence"));

    resultObj["features"] = featuresObj;
    return resultObj.dump();
}

};