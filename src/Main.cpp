#include <iostream>
#include "Engine.h"

int main() {
    Engine engine;

    try {
        engine.launch();
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    return 0;
}