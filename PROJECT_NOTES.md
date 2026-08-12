# Project Notes

## Status (end of 2026-08-12 session)

Pipeline **complete end-to-end**: parse → centroid → AEQD → buffers → union →
inverse-project (densified) → GeoJSON, plus a `contains` predicate, the two-panel
figure, and the V2 geodesic ground-truth probes. **53/53 ctest, `-Wall -Wextra`
clean, ASan+UBSan+LeakSan clean.** CI green on every push.

## Done (green)

- **Parser** — `parsePoint` (`<lat> <N|S> <lon> <E|W>` → signed degrees, range +
  NaN checked) and `parseMission` / `parseMissionFile` (JSON envelope via
  nlohmann/json, pinned `v3.12.0`).
- **Centroid** — unit-vector spherical average (±180° handled; degenerate
  all-cancel case throws).
- **Projection** — mission-centered AEQD via PROJ (`create_crs_to_crs` +
  `normalize_for_visualization`), RAII `PJ`/context, forward/inverse round-trip.
- **Geofence buffers** — GEOS: point + 200 m, route(LineString) + 100 m,
  ROI(Polygon) + 250 m. RAII ownership proven leak-clean.
- **Union** — the three component buffers → the geofence (`GEOSUnion_r`), held as
  a `Geofence` member.
- **Inverse-project + densify** — fence and components back to WGS84;
  `GEOSDensify_r` at 10 m *before* the inverse so long pole-frame edges don't
  smear into wild lon/lat chords. Extent unchanged; longest edge 1374 m → 10 m.
- **`contains(LatLon)`** — projected-space point-in-fence predicate; keeps
  `GEOSGeometry*` out of the public API (resolves the old output-shape TODO).
- **GeoJSON emit** — `toGeoJson`: 7 features (launch / route / ROI inputs + 3
  component buffers + union fence) plus a top-level `aeqd_center` for the viz.
- **Two-panel figure** — `viz/plot_mission.py` → `docs/figure.png`. Panel A = local
  AEQD solution, Panel B = raw lon/lat lesson; `pyproj` rebuilds the AEQD from the
  emitted `aeqd_center`. README + `viz/README.md` walkthroughs written.
- **V2 geodesic probes** — `tests/v2_probe_test.cpp`. `TEST_F` fixture, WGS84
  `geod_init`, ground truth via `geod_direct`/`geod_inverse` (point + polyline/ring
  helpers). Four probes — launch 199/201, route 99/101, ROI 249/251, distant 5 km —
  each geodesic-placed, ground-truthed, then asserted in **projected-space**
  containment.

## Deferred → Friday 2026-08-14 (tagged in TECH_SPEC §9 "If time permits")

- **V1 cross-projection agreement** — re-run the fence in EPSG:3031 and compare to
  the AEQD fence: < 1 m per vertex (Hausdorff), < 0.1% area.
- **V3 structural sanity** — geometry validity (`GEOSisValid`), antimeridian-seam
  artifacts, explicit pole handling — as their own assertions.
- **Figure polish** — legend / label / pole-callout refinements on `docs/figure.png`.
- **Wire `toGeoJson` into `main.cpp`** — the app still runs the old
  `testGeosIntegration` meters-dump; main should parse a mission file → `toGeoJson`
  → stdout/file.

## Design TODOs (flagged, not resolved)

- **throw vs Result** — centroid + geofence `throw` on invariant violations, while
  the parser uses no-exceptions `ParseResult`. Decide/defend the split (recoverable
  → Result; can't-happen invariant → throw). TODO in `centroid.cpp`.

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
- **Densify before inverse-project** — a long AEQD edge inverse-projects to a wild
  lon/lat chord near the pole; densifying at 10 m first fixes rendering without
  moving the geometry.
- **Renderer vs data** — geojson.io / Mapbox use Web Mercator, clipped at
  ±85.0511° (= `atan(sinh(π))`); the pole is literally unrenderable there. The
  emitted GeoJSON is provably correct independent of the viewer — this *is* §8's
  lesson, not a bug.
- **PROJ geodesic API** — `<geodesic.h>` is separate from `<proj.h>`;
  `geod_init`/`geod_direct`/`geod_inverse`, `(lat, lon)` order, azimuth deg cw from
  N. Symbols live in `libproj`, so it links via the already-PUBLIC `PROJ::proj` —
  no extra linking.
- **pyproj** — Python bindings to the *same* PROJ C library;
  `Transformer.from_crs(..., always_xy=True)` for lon/lat order.
- **Visibility** — `PROJ::proj` is PUBLIC (projection.hpp includes proj.h); GEOS is
  kept internal, so `GEOS::geos_c` stays PRIVATE.
- **Parser** — number-abutting-`E` collides with float exponent in `>>`, so the
  format requires a space before the hemisphere letter; NaN slips range checks →
  `std::isnan` guard.
- **Sanitizer gate** — `cmake -S . -B build-asan -DPENGUIN_FENCE_SANITIZE=ON
  -DCMAKE_BUILD_TYPE=Debug && cmake --build build-asan && ctest --test-dir build-asan`.
- **WSL/CMake cache** — Windows-`PATH` bleed fixed via `/etc/wsl.conf`
  (`appendWindowsPath=false`); if a dep resolves to `/mnt/c/...`, `rm -rf build`
  (CMake caches `find_*` results).
