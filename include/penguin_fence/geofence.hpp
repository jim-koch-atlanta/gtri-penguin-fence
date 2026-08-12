#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <geos_c.h> // The GEOS library.

#include "mission.hpp"
#include "projection.hpp"
#include "types.hpp"

namespace penguin_fence {

// The GEOS context handle and every GEOSGeometry* we create must be cleaned up.
// GeosContextPtr and GeomPtr give us RAII.
struct GeosContextDeleter {
    void operator()(GEOSContextHandle_t ctx) const noexcept { GEOS_finish_r(ctx); }
};

struct GeomDeleter {
    GEOSContextHandle_t ctx = nullptr;   // GEOSGeom_destroy_r needs the context, so the deleter carries it.
    void operator()(GEOSGeometry* g) const noexcept { if (g) GEOSGeom_destroy_r(ctx, g); }
};

using GeosContextPtr = std::unique_ptr<std::remove_pointer_t<GEOSContextHandle_t>, GeosContextDeleter>;
using GeomPtr = std::unique_ptr<GEOSGeometry, GeomDeleter>;

// The geofence as a collection of WGS84 lat/lon points.
struct GeofenceAsLatLonPoints {
    std::vector<LatLon> launchPointFence;
    std::vector<LatLon> ingressRouteFence;
    std::vector<LatLon> regionOfInterestFence;
    std::vector<LatLon> mergedFence;
};

// A mission geofence: the union of a 200 m buffer around the launch point, a
// 100 m buffer around the ingress route, and a 250 m buffer around the ROI,
// computed in a mission-centered AEQD projection. Built at construction.
class Geofence {
    // ctx is declared first so it outlives the GEOS geometries below: they are
    // freed through the ctx handle, so the context must still be alive when they
    // destruct. (proj is PROJ-only and independent of the GEOS context.)
    GeosContextPtr ctx;
    std::string errorMsg;
    Projection proj;

    // The three component buffers and their union, all in projected meters.
    GeomPtr launchBuffer;
    GeomPtr ingressBuffer;
    GeomPtr roiBuffer;
    GeomPtr fence;

    GeomPtr makePoint(const Point2D& pt);
    GeomPtr makeLineString(const std::vector<Point2D>& pts);
    GeomPtr makePolygon(const std::vector<Point2D>& pts);
    GeomPtr makeBuffer(const GEOSGeometry* geometry, double distance, int segmentsPerQuadrant = 10);
    GeomPtr makeUnion(const GEOSGeometry* a, const GEOSGeometry* b);
    GeomPtr densify(const GEOSGeometry* geometry, double maxEdge);

    std::vector<LatLon> ringToLatLon(const GEOSGeometry* ring);
    std::vector<LatLon> polygonToLatLon(const GEOSGeometry* poly);

    public:

    explicit Geofence(const MissionData& mission);

    // True if the given WGS84 point lies inside the geofence.
    bool contains(const LatLon& point);

    // Outputs the generated geofence as a collection of WGS84 lat / lon points.
    GeofenceAsLatLonPoints asLatLonPoints();
};

}  // namespace penguin_fence
