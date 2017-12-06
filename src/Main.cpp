#include <iostream>
#include "Engine.h"
#include "Frames/TestFrame.h"

int main() {
    Engine engine;
    TestFrame frame(engine);

    try {
        engine.changeFrame<TestFrame>(frame);
        engine.launch();
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    return 0;
}