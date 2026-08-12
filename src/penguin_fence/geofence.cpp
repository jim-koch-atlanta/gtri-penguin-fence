#include <stdexcept>
#include <string>
#include <vector>

#include <geos_c.h> // The GEOS library.

#include "penguin_fence/centroid.hpp"
#include "penguin_fence/geofence.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/projection.hpp"
#include "penguin_fence/types.hpp"

// Buffer standoff distances, in meters (spec: launch 200 / route 100 / ROI 250).
static const double LAUNCH_POINT_BUFFER = 200.0;
static const double INGRESS_ROUTE_BUFFER = 100.0;
static const double REGION_OF_INTEREST_BUFFER = 250.0;

namespace penguin_fence {

// GEOSMessageHandler_r: void(const char* message, void* userdata)
static void onGeosError(const char* message, void* userdata) {
    *static_cast<std::string*>(userdata) = message;
}

// Project a list of WGS84 lat/lons into local AEQD meters.
static std::vector<Point2D> project(Projection& proj, const std::vector<LatLon>& latLons) {
    std::vector<Point2D> result;
    result.reserve(latLons.size());
    for (const LatLon& latLon : latLons) {
        result.push_back(proj.latLonToEastingNorthing(latLon));
    }
    return result;
}

Geofence::Geofence(const MissionData& mission)
  : ctx{ GEOS_init_r() }
  , proj{ calculateCentroid(mission) } {

    if (!ctx) {
        throw std::runtime_error("GEOS_init_r() failed");
    }

    // GEOS reports errors through a callback rather than an errno.
    GEOSContext_setErrorMessageHandler_r(ctx.get(), onGeosError, &errorMsg);

    // Build each mission geometry in local meters, then buffer it by its standoff.
    GeomPtr launchGeom = makePoint(proj.latLonToEastingNorthing(mission.launchPoint));
    launchBuffer = makeBuffer(launchGeom.get(), LAUNCH_POINT_BUFFER);

    GeomPtr routeGeom = makeLineString(project(proj, mission.ingressRoute));
    ingressBuffer = makeBuffer(routeGeom.get(), INGRESS_ROUTE_BUFFER);

    GeomPtr roiGeom = makePolygon(project(proj, mission.regionOfInterest));
    roiBuffer = makeBuffer(roiGeom.get(), REGION_OF_INTEREST_BUFFER);

    // Union the three component buffers into the geofence.
    GeomPtr partial = makeUnion(launchBuffer.get(), ingressBuffer.get());
    fence = makeUnion(partial.get(), roiBuffer.get());
}

GeomPtr Geofence::makePoint(const Point2D& pt) {
    GEOSCoordSequence* seq = GEOSCoordSeq_create_r(ctx.get(), 1, 2);
    if (!seq) throw std::runtime_error("coordseq create failed: " + errorMsg);

    GEOSCoordSeq_setXY_r(ctx.get(), seq, 0, pt.easting, pt.northing);

    GEOSGeometry* point = GEOSGeom_createPoint_r(ctx.get(), seq);   // takes ownership of seq
    if (!point) throw std::runtime_error("createPoint failed: " + errorMsg);
    return GeomPtr{ point, GeomDeleter{ ctx.get() } };
}

GeomPtr Geofence::makeLineString(const std::vector<Point2D>& pts) {
    GEOSCoordSequence* seq = GEOSCoordSeq_create_r(ctx.get(), static_cast<unsigned>(pts.size()), 2);
    if (!seq) throw std::runtime_error("coordseq create failed: " + errorMsg);

    for (std::size_t i = 0; i < pts.size(); ++i)
        GEOSCoordSeq_setXY_r(ctx.get(), seq, static_cast<unsigned>(i), pts[i].easting, pts[i].northing);

    GEOSGeometry* line = GEOSGeom_createLineString_r(ctx.get(), seq);   // takes ownership of seq
    if (!line) throw std::runtime_error("createLineString failed: " + errorMsg);
    return GeomPtr{ line, GeomDeleter{ ctx.get() } };
}

GeomPtr Geofence::makePolygon(const std::vector<Point2D>& pts) {
    GEOSCoordSequence* seq = GEOSCoordSeq_create_r(ctx.get(), static_cast<unsigned>(pts.size()), 2);
    if (!seq) throw std::runtime_error("coordseq create failed: " + errorMsg);

    for (std::size_t i = 0; i < pts.size(); ++i)
        GEOSCoordSeq_setXY_r(ctx.get(), seq, static_cast<unsigned>(i), pts[i].easting, pts[i].northing);

    // A linear ring must be closed (first == last); the ROI is supplied closed.
    GEOSGeometry* ring = GEOSGeom_createLinearRing_r(ctx.get(), seq);   // takes ownership of seq
    if (!ring) throw std::runtime_error("createLinearRing failed: " + errorMsg);

    GEOSGeometry* polygon = GEOSGeom_createPolygon_r(ctx.get(), ring, nullptr, 0);  // takes ownership of ring
    if (!polygon) throw std::runtime_error("createPolygon failed: " + errorMsg);    // do NOT free ring
    return GeomPtr{ polygon, GeomDeleter{ ctx.get() } };
}

GeomPtr Geofence::makeBuffer(const GEOSGeometry* geometry, double distance, int segmentsPerQuadrant) {
    // GEOSBuffer_r reads its input and returns a NEW geometry; it does not consume it.
    GEOSGeometry* buffered = GEOSBuffer_r(ctx.get(), geometry, distance, segmentsPerQuadrant);
    if (!buffered) throw std::runtime_error("buffer failed: " + errorMsg);
    return GeomPtr{ buffered, GeomDeleter{ ctx.get() } };
}

GeomPtr Geofence::makeUnion(const GEOSGeometry* a, const GEOSGeometry* b) {
    // GEOSUnion_r returns a NEW geometry; it does not consume its inputs, so the
    // component buffers stay owned by their members.
    GEOSGeometry* united = GEOSUnion_r(ctx.get(), a, b);
    if (!united) throw std::runtime_error("union failed: " + errorMsg);
    return GeomPtr{ united, GeomDeleter{ ctx.get() } };
}

bool Geofence::contains(const LatLon& point) {
    GeomPtr queryPoint = makePoint(proj.latLonToEastingNorthing(point));
    char result = GEOSContains_r(ctx.get(), fence.get(), queryPoint.get());  // 1 yes, 0 no, 2 exception
    if (result == 2) throw std::runtime_error("GEOSContains failed: " + errorMsg);
    return result == 1;
}

}  // namespace penguin_fence
