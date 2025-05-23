#include "version.h"
#include <iostream>

int main() {
    std::cout << PROJECT_VERSION << '\n'
              << PROJECT_SHA << '\n';

    std::cout << "hello world\n";
    return 0;
}