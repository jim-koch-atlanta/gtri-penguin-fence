# Skeleton Notes

End-of-session state (2026-08-10). The walking skeleton is complete and green;
tomorrow starts on geometry.

## What's green

- **Build** — CMake C++20, publishable-library layout (`include/` + `src/`),
  nested per-directory `CMakeLists`. Configures + builds clean (cmake 4.2 / g++ 15.2).
- **Tests** — GoogleTest via FetchContent (pinned `v1.16.0`); `ctest` **5/5**.
- **CI** — GitHub Actions on every push: apt-installs PROJ/GEOS/GDAL, configures,
  builds, runs `ctest`. (Pieces 1–3 confirmed green on push; Piece 4 green locally,
  pending its first push.)
- **Toolchain proven** — PROJ 9.7 / GEOS 3.14 / GDAL 3.12 all found via
  `find_package` and linked to the library; a "links and breathes" test asserts each
  reports a non-empty version. No geometry yet — this exists so tomorrow starts with
  the toolchain proven, not fought.

## Tomorrow's entry point

**Parser + hemisphere-suffix parsing test (TECH_SPEC §7).** First real code:

1. `include/penguin_fence/parser.hpp` + `src/penguin_fence/parser.cpp` — parse
   mission geometry: WGS84 given as latitude-then-longitude with hemisphere
   suffixes (N/S/E/W) → signed decimal degrees. Inputs per §2: a launch point, a
   3-vertex ingress polyline, a 5-vertex closed ROI polygon (first == last).
2. `tests/parser_test.cpp` — the §7 hemisphere-suffix parsing test: `S`/`W`
   negate, round-trip, malformed-input handling.

Then follow the §4 pipeline: unit-vector spherical centroid (with the ±180° case
test from §7 — averaging 179°E and 179°W must NOT yield ~0°) → construct local
AEQD via PROJ → project → GEOS buffer → union → inverse-project → GeoJSON.

## Gotchas carried over

- **GEOS needs a context for real work.** `GEOSversion()` (the smoke test) is one
  of the few C-API calls needing no setup. Buffer/union/WKT require a GEOS context
  handle — use the reentrant API `GEOS_init_r()` + `*_r` functions with notice/error
  callbacks. Set that up first when geometry starts.
- **PROJ runtime data.** `proj_info()` needs no database, but real transforms need
  `proj.db` (ships with `libproj-dev`). A "proj.db not found" error → check `PROJ_LIB`.
- **WSL/CMake cache (resolved).** The Windows-`PATH` bleed is fixed persistently via
  `/etc/wsl.conf` (`appendWindowsPath=false`). If a dependency ever resolves to a
  `/mnt/c/...` path again, `rm -rf build` — CMake caches `find_*` results, so a bare
  reconfigure keeps the stale hit.
- **GDAL include idiom.** Prefer `#include <gdal.h>` (the `GDAL::GDAL` target adds the
  include dir) over `<gdal/gdal.h>` (a Debian-specific path) for portability.
