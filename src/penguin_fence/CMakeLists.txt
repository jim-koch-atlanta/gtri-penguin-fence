# --- Find Dependencies ---
# Locate PROJ, GEOS, and GDAL on the system.
# REQUIRED ensures configuration fails immediately if they are missing.
find_package(PROJ REQUIRED)
find_package(GEOS REQUIRED)
find_package(GDAL REQUIRED)

# --- Core library: all production geofence logic lands here ---
add_library(penguin_fence
    version.cpp)

# Public API headers live in the top-level include/ tree, exposed to
# consumers (the app, the tests) as "penguin_fence/<name>.hpp".
target_include_directories(penguin_fence PUBLIC
    ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(penguin_fence
    PRIVATE
        penguin_fence_warnings
        PROJ::proj
        GEOS::geos_c
        GDAL::GDAL)
