#include <iostream>

#include "penguin_fence/version.hpp"

int main() {
    std::cout << "penguin_fence " << penguin_fence::version() << '\n';
    return 0;
}
