# Project Notes

## Status (end of 2026-08-11 session)

Pipeline built through the buffers. **45/45 ctest, `-Wall -Wextra` clean,
ASan+UBSan+LeakSan clean.** CI green on every push.

## Done

- **Parser** — `parsePoint` (`<lat> <N|S> <lon> <E|W>` → signed degrees, range +
  NaN checked) and `parseMission` / `parseMissionFile` (JSON envelope via
  nlohmann/json, pinned `v3.12.0`).
- **Centroid** — unit-vector spherical average (±180° handled; degenerate
  all-cancel case throws).
- **Projection** — mission-centered AEQD via PROJ (`create_crs_to_crs` +
  `normalize_for_visualization`), RAII `PJ`/context, forward/inverse round-trip.
- **Geofence buffers** — GEOS: point + 200 m, route(LineString) + 100 m,
  ROI(Polygon) + 250 m. RAII ownership proven leak-clean. Buffers held as
  `Geofence` members.

## Next entry point — step 6: union

1. **Union** the three component buffers → the geofence (`GEOSUnaryUnion_r` on a
   collection, or chained `GEOSUnion_r`). Store as a member.
2. Then the deferred **step-5 boundary spot-check test** — needs the result
   observable: add an accessor that does NOT leak `GEOSGeometry*` into the public
   API (a predicate like `contains(LatLon)`, or emit WKT/coords). Checks:
   99/101 m route, 199/201 m launch, 249/251 m ROI, one distant point outside all.
3. Remaining §4/§7: (7) inverse-project the fence to WGS84; (8) GeoJSON emit;
   (9) V2 geodesic probes via PROJ geod; (10) V1 EPSG:3031 cross-check
   (<1 m/vertex, <0.1% area); (11) V3 validity + antimeridian-seam.

## Design TODOs (flagged, not resolved)

- **throw vs Result** — centroid + geofence `throw` on invariant violations,
  while the parser uses no-exceptions `ParseResult`. Decide/defend the split
  (recoverable → Result; can't-happen invariant → throw). TODO in `centroid.cpp`.
- **Geofence output shape** — `Generate()` stores the 3 buffers privately with no
  accessor; the public result (union → inverse → GeoJSON) is steps 6–8. Design the
  interface then, keeping `GEOSGeometry*` out of it.

## Gotchas learned

- **GEOS ownership** — `createPoint/LineString/LinearRing/Polygon` *consume* their
  input (coordseq/ring) even on failure. Never free it after handing it off; on
  NULL just throw. `GEOSBuffer_r` does NOT consume (returns a new geom).
- **GEOS RAII** — `GEOSGeom_destroy_r` needs the context, so the deleter carries
  the ctx handle; declare the context member *before* geometry members so it
  outlives them.
- **unique_ptr of pointer typedefs** — `GEOSContextHandle_t` and `GEOSGeom` are
  already pointers; use `remove_pointer_t<GEOSContextHandle_t>` and `GEOSGeometry`
  (the struct) as element types, or you manage a pointer-to-pointer.
- **PROJ axis order** — `EPSG:4326` is lat,lon; `proj_normalize_for_visualization`
  gives (lon,lat)↔(E,N). `crs_to_crs` works in degrees, not radians.
- **Visibility** — `PROJ::proj` is PUBLIC (projection.hpp includes proj.h);
  GEOS is kept internal, so `GEOS::geos_c` stays PRIVATE.
- **Parser** — number-abutting-`E` collides with float exponent in `>>`, so the
  format requires a space before the hemisphere letter; NaN slips range checks →
  `std::isnan` guard.
- **Sanitizer gate** — `cmake -S . -B build-asan -DPENGUIN_FENCE_SANITIZE=ON
  -DCMAKE_BUILD_TYPE=Debug && cmake --build build-asan && ctest --test-dir build-asan`.
- **WSL/CMake cache** — Windows-`PATH` bleed fixed via `/etc/wsl.conf`
  (`appendWindowsPath=false`); if a dep resolves to `/mnt/c/...`, `rm -rf build`
  (CMake caches `find_*` results).
