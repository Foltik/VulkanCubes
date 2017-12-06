#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkTraits.h"
#include "Instance.h"

namespace vk::khr {
    class Surface : public VkTraits<Surface, VkSurfaceKHR> {
    private:
        VkSurfaceKHR m_surface = {};
        VkInstance m_instance = {};

    public:
        Surface() = default;
        Surface(vk::Instance& instance, GLFWwindow* window);

        VkSurfaceKHR vkSurfaceKHR() const;
	};
}
