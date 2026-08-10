# The library is a self-contained unit: its build lives with its sources.
add_subdirectory(penguin_fence)

add_executable(penguin_fence_app main.cpp)

target_link_libraries(penguin_fence_app
    PRIVATE penguin_fence penguin_fence_warnings)
