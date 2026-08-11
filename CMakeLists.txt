cmake_minimum_required(VERSION 3.24)

project(penguin_fence
    VERSION 0.1.0
    DESCRIPTION "Operation Waddle Watch"
    LANGUAGES CXX)

# C++20, no compiler-specific extensions ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit build/compile_commands.json so editor tooling (IntelliSense, clangd,
# clang-tidy) sees the exact compiler flags -- including dependency include
# dirs like GDAL's -- instead of guessing.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pick a sensible default build type for single-config generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Optional AddressSanitizer + UndefinedBehaviorSanitizer build. Configure a
# separate dir with -DPENGUIN_FENCE_SANITIZE=ON (pair with -DCMAKE_BUILD_TYPE=Debug
# for clean traces). Applied globally so fetched deps are instrumented too.
option(PENGUIN_FENCE_SANITIZE "Build with ASan + UBSan" OFF)
if(PENGUIN_FENCE_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# INTERFACE target that is applied to OUR code only (never to fetched dependencies like GoogleTest).
add_library(penguin_fence_warnings INTERFACE)
target_compile_options(penguin_fence_warnings INTERFACE -Wall -Wextra)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
