#!/usr/bin/env python3
"""Two-panel figure for the Operation Waddle Watch mission geofence.

Panel A -- "the solution": every feature re-projected into the mission's local
Azimuthal Equidistant (AEQD) meters frame -- the honest view, where the fence is
the ~2.3 km shape it actually is.

Panel B -- "the lesson": the SAME GeoJSON plotted as raw lon/lat, which smears a
2.3 km fence across the whole longitude axis. That is why you cannot do geometry
in lat/lon near the pole.

Reads a mission GeoJSON (default build/mission.geojson) whose top-level
`aeqd_center` gives the projection center, and writes docs/figure.png.

    python viz/plot_mission.py [mission.geojson] [figure.png]
"""
import json
import sys

import matplotlib
matplotlib.use("Agg")              # headless: write a PNG, no display needed
import matplotlib.pyplot as plt
from pyproj import Transformer

IN_PATH = sys.argv[1] if len(sys.argv) > 1 else "build/mission.geojson"
OUT_PATH = sys.argv[2] if len(sys.argv) > 2 else "docs/figure.png"

doc = json.load(open(IN_PATH))

# Rebuild the SAME local AEQD frame the C++ used, from the emitted center.
center = doc["aeqd_center"]
aeqd = (f"+proj=aeqd +lat_0={center['lat']} +lon_0={center['lon']} "
        f"+datum=WGS84 +units=m +type=crs")
to_m = Transformer.from_crs("EPSG:4326", aeqd, always_xy=True)   # (lon,lat) -> (x,y) m

features = {f["properties"]["name"]: f for f in doc["features"]}


def ring_lonlat(name):
    """Exterior coordinates of a named feature as a list of (lon, lat)."""
    g = features[name]["geometry"]
    c = g["coordinates"]
    if g["type"] == "Point":
        return [c]
    if g["type"] == "LineString":
        return c
    return c[0]                    # Polygon: exterior ring


def to_meters(pts):
    xs, ys = [], []
    for lon, lat in pts:
        x, y = to_m.transform(lon, lat)
        xs.append(x)
        ys.append(y)
    return xs, ys


COMPONENTS = {
    "fence_launch_point":       ("tab:blue",   "launch buffer (200 m)"),
    "fence_ingress_route":      ("tab:green",  "route buffer (100 m)"),
    "fence_region_of_interest": ("tab:orange", "ROI buffer (250 m)"),
}

fig, (axA, axB) = plt.subplots(1, 2, figsize=(15, 7.5))

# ---------- Panel A: local AEQD meters (the solution) ----------
for name, (color, label) in COMPONENTS.items():
    xs, ys = to_meters(ring_lonlat(name))
    axA.fill(xs, ys, color=color, alpha=0.25, label=label)

fx, fy = to_meters(ring_lonlat("fence"))
axA.plot(fx, fy, color="black", lw=2.0, label="geofence (union)")

rx, ry = to_meters(ring_lonlat("ingress_route"))
axA.plot(rx, ry, color="red", lw=1.5, marker="o", ms=4, label="ingress route")

ox, oy = to_meters(ring_lonlat("region_of_interest"))
axA.plot(ox, oy, color="purple", lw=1.5, ls="--", label="ROI polygon")

lx, ly = to_meters(ring_lonlat("launch_point"))
axA.plot(lx, ly, marker="*", color="black", ms=15, ls="none", label="launch")

# The South Pole (longitude is arbitrary there).
spx, spy = to_m.transform(0.0, -90.0)
axA.plot(spx, spy, marker="x", color="crimson", ms=12, mew=2.5, ls="none")
axA.annotate("South Pole", (spx, spy), textcoords="offset points",
             xytext=(8, -4), color="crimson", fontsize=9)

axA.set_aspect("equal", adjustable="datalim")
axA.set_xlabel("easting (m)")
axA.set_ylabel("northing (m)")
axA.set_title("Mission geofence — local AEQD frame")
axA.grid(True, ls=":", alpha=0.6)
axA.legend(loc="upper left", fontsize=8)

# ---------- Panel B: raw lon/lat (the lesson) ----------
for name, (color, _) in COMPONENTS.items():
    lons, lats = zip(*ring_lonlat(name))
    axB.fill(lons, lats, color=color, alpha=0.25)
lons, lats = zip(*ring_lonlat("fence"))
axB.plot(lons, lats, color="black", lw=1.0)

axB.set_xlim(-180, 180)
axB.set_xlabel("longitude (deg)")
axB.set_ylabel("latitude (deg)")
axB.set_title("Same data as raw lat/lon — why naive geometry fails at the pole")
axB.grid(True, ls=":", alpha=0.6)

fig.tight_layout()
fig.savefig(OUT_PATH, dpi=120)
print(f"wrote {OUT_PATH}")
