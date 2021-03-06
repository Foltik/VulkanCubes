#include "Engine.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>

#include "Threads/Scheduler.h"

namespace {
    GLFWwindow* createWindow(const std::string& title, int width, int height) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        return glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    }
}

Engine::Engine() : scheduler(),
                   m_window(800, 600, "OpenMiner"),
                   context(m_window, g_debug) {
}

Engine::~Engine() {
}

void Engine::launch() {
    while (!glfwWindowShouldClose(m_window.window())) {
        glfwPollEvents();

        if (!m_frames.empty()) {
            getFrame().update(0, context);
            getFrame().render(context);
        }
    }
}

void Engine::popFrame() {
    getFrame().leave();
    m_frames.pop_back();
    if (!m_frames.empty())
        getFrame().enter();
}

Frame& Engine::getFrame() {
    return *m_frames.back();
}

Window& Engine::window() {
    return m_window;
}

