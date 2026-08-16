#include <cstdio>
#include <fstream>
#include <iostream>
#include <string_view>

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/geofence.hpp"
#include "penguin_fence/geojson.hpp"
#include "penguin_fence/parser.hpp"
#include "penguin_fence/projection.hpp"
#include "penguin_fence/version.hpp"

using namespace penguin_fence;

void testGeosIntegration(const MissionData& m) {
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

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <input_json_path> <output_geojson_path>\n", argv[0]);
        return 1;
    }
    
    const std::string inPath = argv[1];
    const std::string outPath = argv[2];
    std::cout << "Input file path:  " << inPath << "\n";
    std::cout << "Output file path: " << outPath << "\n\n";

    auto parsed = parseMissionFile(inPath);   // ParseResult — check .ok()/.error()
    if (!parsed.ok()) {
        std::fprintf(stderr, "Error parsing mission file: %s\n", parsed.error().c_str());
        return 1;
    }

    const MissionData& m = parsed.value();
    testGeosIntegration(m);
    
    std::string geojson;
    try {
        Geofence fence{ m };                                 // builds the whole pipeline
        geojson = toGeoJson(m, fence.asLatLonPoints());
    }
    catch (const std::exception& e) {
        std::fprintf(stderr, "geofence failed: %s\n", e.what());
        return 1;
    }

    std::ofstream file(outPath);
    if (!file) {
        std::fprintf(stderr, "cannot open %s\n", outPath.c_str()); return 1;
    }
    file << geojson;
    std::cout << "Successfully wrote output to " << outPath << "\n";
    
    return 0;
}
