#pragma once

#include <string>

#include <proj.h> // The PROJ library.

#include "types.hpp"


namespace penguin_fence {

// PJ and PJ_CONTEXT objects must be cleaned up. ProjPtr and ContextPtr give us RAII.
struct ProjDeleter    { void operator()(PJ* p) const noexcept { proj_destroy(p); } };
struct ContextDeleter { void operator()(PJ_CONTEXT* c) const noexcept { proj_context_destroy(c); } };
using ProjPtr    = std::unique_ptr<PJ, ProjDeleter>;
using ContextPtr = std::unique_ptr<PJ_CONTEXT, ContextDeleter>;

class Projection {
    // Declared context-first so it outlives proj: members destruct in reverse
    // declaration order, so proj (which belongs to ctx) is freed first.
    ContextPtr ctx;
    ProjPtr proj;
    LatLon centroid;

    static std::string getAeqdString(LatLon latLon);

    public:

    explicit Projection(const LatLon& centroid);

    Point2D latLonToEastingNorthing(const LatLon& latLon);
    LatLon eastingNorthingToLatLon(const Point2D& p);
};

}  // namespace penguin_fence
