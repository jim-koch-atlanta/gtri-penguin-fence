#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <geos_c.h> // The GEOS library.

#include "mission.hpp"
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

class Geofence {
    // ctx is declared first so it outlives the geometries below: they are freed
    // through the ctx handle, so the context must still be alive when they destruct.
    GeosContextPtr ctx;
    std::string errorMsg;

    // The three component buffers, populated by Generate().
    GeomPtr launchBuffer;
    GeomPtr ingressBuffer;
    GeomPtr roiBuffer;

    GeomPtr makePoint(const Point2D& pt);
    GeomPtr makeLineString(const std::vector<Point2D>& pts);
    GeomPtr makePolygon(const std::vector<Point2D>& pts);
    GeomPtr makeBuffer(const GEOSGeometry* geometry, double distance, int segmentsPerQuadrant = 10);

    public:

    explicit Geofence();

    void Generate(const MissionData& mission);
};

}  // namespace penguin_fence
