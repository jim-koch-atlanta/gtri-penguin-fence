#include <gtest/gtest.h>

#include "penguin_fence/version.hpp"

// Proof of life: production code is reachable and callable from the tests.
TEST(Version, IsNotEmpty) {
    EXPECT_FALSE(penguin_fence::version().empty());
}
