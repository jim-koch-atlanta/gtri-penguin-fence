# Penguin Fence

Geofence computation for *Operation Waddle Watch*. Given a launch point, an ingress route, and a region-of-interest polygon in WGS84, the tool computes the union of buffered standoff zones and emits it as GeoJSON.

The full algorithm and verification plan live in **[docs/TECH_SPEC.md](docs/TECH_SPEC.md)**.

## Build & test

CMake ≥ 3.24 and a C++20 compiler.

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Layout

| Path                   | Description                                 |
| ---------------------- | ------------------------------------------- |
| docs/TECH_SPEC.md      | algorithm + verification plan               |
| include/penguin_fence/ | public headers -- the library's API surface |
| src/penguin_fence/     | library implementation                      |
| src/main.cpp           | thin application entry point                |
| tests/                 | GoogleTest suite                            |
| viz/                   | visualization scripts (Python later)        |
| .github/workflows/     | CI (added in a later step)                  |

## Design Decisions

- **Publishable-library layout (`include/` + `src/` split).** Public headers live in `include`, implementation in `src`, so the library presents a clean, self-contained API surface. This is the shape a real distributable C++ library takes. This is **intentionally overkill** for a nine-vertex take-home. However, it's adopted for clarity of intent and because it costs almost nothing when done from the beginning. (I intentionally skipped doing a full `install()` / export packaging, which _would_ be overkill at this size.)

- **Nested CMake.** The root `CMakeLists.txt` owns project-wide setup (C++20, warnings, `enable_testing()`); each directory owns the targets built from its own sources, so the build layout mirrors the source tree.

- **Strict warnings scoped to our code.** `-Wall -Wextra` are carried on an `INTERFACE` target linked only to our targets, so fetched dependencies (GoogleTest now, PROJ/GEOS/GDAL headers later) don't trip our warning budget.

- **GoogleTest via FetchContent** Reproducible test builds with no system dependency.

* **C++20**, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`), for portability.
