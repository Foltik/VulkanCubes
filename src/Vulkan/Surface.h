#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkTraits.h"

namespace vk::khr {
    class Surface : public VkTraits<Surface, VkSurfaceKHR> {
    private:
        VkSurfaceKHR m_surface = {};

    public:
        Surface(vk::Instance instance, GLFWwindow* window) {
            glfwCreateWindowSurface(instance.vkInstance(), window, nullptr, &m_surface);
        }
    };
}