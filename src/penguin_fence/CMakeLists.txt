# --- Find Dependencies ---
# Locate PROJ, GEOS, and GDAL on the system.
# REQUIRED ensures configuration fails immediately if they are missing.
find_package(PROJ REQUIRED)
find_package(GEOS REQUIRED)
find_package(GDAL REQUIRED)

include(FetchContent)

# Fetch nlohmann/json.hpp for parsing of mission data.
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

# --- Core library: all production geofence logic lands here ---
add_library(penguin_fence
    centroid.cpp
    parser.cpp
    projection.cpp
    version.cpp)

# Public API headers live in the top-level include/ tree, exposed to
# consumers (the app, the tests) as "penguin_fence/<name>.hpp".
target_include_directories(penguin_fence PUBLIC
    ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(penguin_fence
    PRIVATE
        penguin_fence_warnings
        GEOS::geos_c
        GDAL::GDAL
        nlohmann_json::nlohmann_json
)

target_link_libraries(penguin_fence
    PUBLIC
        PROJ::proj
)
