#include <format>
#include <memory>
#include <stdexcept>
#include <string>

#include <proj.h> // The PROJ library.

#include "penguin_fence/projection.hpp"
#include "penguin_fence/types.hpp"

namespace penguin_fence {

Projection::Projection(const LatLon& centroid)
  : ctx{ proj_context_create() }
  , centroid{ centroid } {

    if (!ctx) {
        throw std::runtime_error("proj_context_create failed");
    }

    // EPSG:4326 is lat/lon order; we normalize below to lon/lat.
    ProjPtr rawProj{ proj_create_crs_to_crs(
        ctx.get(),
        "EPSG:4326",
        getAeqdString(centroid).c_str(),
        nullptr) };

    if (!rawProj) {
        int e = proj_context_errno(ctx.get());
        throw std::runtime_error(std::string("proj_create_crs_to_crs failed: ") +
                                    proj_context_errno_string(ctx.get(), e));
    }

    // Normalize to (lon, lat) <-> (easting, northing).
    proj = ProjPtr{ proj_normalize_for_visualization(ctx.get(), rawProj.get()) };

    if (!proj) {
        int e = proj_context_errno(ctx.get());
        throw std::runtime_error(std::string("proj_normalize_for_visualization failed: ") +
                                    proj_context_errno_string(ctx.get(), e));
    }
}

std::string Projection::getAeqdString(LatLon latLon) {
    return "+proj=aeqd +lat_0=" + std::format("{:.12f}", latLon.lat) +
        " +lon_0=" + std::format("{:.12f}", latLon.lon) +
        " +datum=WGS84 +units=m +type=crs";
}

Point2D Projection::latLonToEastingNorthing(const LatLon& latLon) {
    proj_errno_reset(proj.get());

    // Post-normalize: x=lon, y=lat (degrees).
    PJ_COORD in = proj_coord(latLon.lon, latLon.lat, 0, 0);
    PJ_COORD out = proj_trans(proj.get(), PJ_FWD, in);   // WGS84 -> AEQD meters

    if (proj_errno(proj.get()) != 0) {
        int e = proj_errno(proj.get());
        throw std::runtime_error(std::string("forward transform failed: ") +
                                    proj_context_errno_string(ctx.get(), e));
    }

    return Point2D{ .easting = out.xy.x, .northing = out.xy.y };
}

LatLon Projection::eastingNorthingToLatLon(const Point2D& p) {
    proj_errno_reset(proj.get());

    // Post-normalize: x=easting, y=northing.
    PJ_COORD in = proj_coord(p.easting, p.northing, 0, 0);
    PJ_COORD out = proj_trans(proj.get(), PJ_INV, in);   // AEQD meters -> WGS84

    if (proj_errno(proj.get()) != 0) {
        int e = proj_errno(proj.get());
        throw std::runtime_error(std::string("inverse transform failed: ") +
                                    proj_context_errno_string(ctx.get(), e));
    }

    return LatLon{ .lat = out.xy.y, .lon = out.xy.x };
}

}
