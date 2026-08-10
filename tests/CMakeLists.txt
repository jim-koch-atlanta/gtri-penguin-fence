include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
    GIT_SHALLOW    TRUE)

# Keep GoogleTest out of `install`, and use the shared CRT on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(penguin_fence_tests
    smoke_test.cpp
    version_test.cpp)

target_link_libraries(penguin_fence_tests
    PRIVATE
        penguin_fence
        penguin_fence_warnings
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(penguin_fence_tests)
