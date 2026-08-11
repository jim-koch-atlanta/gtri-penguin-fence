#pragma once

namespace penguin_fence {

/**
 * A 2D geographical point. Signed decimal degrees, WGS84.
 */
struct LatLon {
    double lat;
    double lon;
};

/**
 * A 3D Cartesian point. Signed decimal values.
 */
struct Point3D {
    double x;
    double y;
    double z;
};

struct Point2D {
    double easting;
    double northing;
};

}
