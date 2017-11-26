#include "Window.h"

void Window::createWindow(int width, int height, const std::string& title) {
    m_width = width;
    m_height = height;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

Window::Window() {
    createWindow(1280, 720, "OpenMiner");
}

Window::Window(int width, int height, const std::string& title) {
    createWindow(width, height, title);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

GLFWwindow* Window::window() {
    return m_window;
}

int Window::width() {
    return m_width;
}

int Window::height() {
    return m_height;
}
