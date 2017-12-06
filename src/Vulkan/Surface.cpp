#include "Surface.h"

using namespace vk;

khr::Surface::Surface(Instance& instance, GLFWwindow* window) {
    m_instance = instance.vkInstance();
    glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
}

VkSurfaceKHR khr::Surface::vkSurfaceKHR() const {
    return m_surface;
}

