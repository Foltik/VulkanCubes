#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class Window {
public:
    Window();
    Window(int width, int height, const std::string& title);
    ~Window();

    GLFWwindow* window();
    int width();
    int height();

private:
    void createWindow(int width, int height, const std::string& title);

    GLFWwindow* m_window;
    int m_width;
    int m_height;
};


