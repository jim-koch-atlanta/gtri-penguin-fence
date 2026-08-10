#include <gtest/gtest.h>

#include "penguin_fence/version.hpp"

// Proof of life: production code is reachable and callable from the tests.
TEST(Version, IsNotEmpty) {
    EXPECT_FALSE(penguin_fence::version().empty());
}

// PROJ is properly hooked up.
TEST(Version, ProjIsNotEmpty) {
    EXPECT_FALSE(penguin_fence::proj_version().empty());
}

// GEOS is properly hooked up.
TEST(Version, GeosIsNotEmpty) {
    EXPECT_FALSE(penguin_fence::geos_version().empty());
}

// GDAL is properly hooked up.
TEST(Version, GdalIsNotEmpty) {
    EXPECT_FALSE(penguin_fence::gdal_version().empty());
}
