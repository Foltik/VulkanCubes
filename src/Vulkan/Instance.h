#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util.h"
#include "VkTraits.h"
#include "Structure.h"

namespace vk::khr {
    class Surface;
}

namespace vk {
    class ApplicationInfo : public VkTraits<ApplicationInfo, VkApplicationInfo> {
    private:
        const Structure sType = Structure::Application;

    public:
        const void* pNext = nullptr;
        const char* pApplicationName = nullptr;
        uint32_t applicationVersion = 0;
        const char* pEngineName = nullptr;
        uint32_t engineVersion = 0;
        uint32_t apiVersion = 0;

        constexpr explicit ApplicationInfo(const char* pApplicationName, uint32_t apiVersion = VK_API_VERSION_1_0)
            : pApplicationName(pApplicationName),
              apiVersion(apiVersion) {}

    };


    class InstanceInfo : public VkTraits<InstanceInfo, VkInstanceCreateInfo> {
    private:
        const Structure sType = Structure::Instance;
    public:
        const void* pNext = nullptr;
        VkInstanceCreateFlags flags = {};
        const VkApplicationInfo* pApplicationInfo = nullptr;
        uint32_t enabledLayerCount = 0;
        const char* const* ppEnabledLayerNames = nullptr;
        uint32_t enabledExtensionCount = 0;
        const char* const* ppEnabledExtensionNames = nullptr;

        constexpr explicit InstanceInfo(const ApplicationInfo& appInfo)
            : pApplicationInfo(&appInfo) {}

        void setLayers(const std::vector<const char*>& layers);
        void setExtensions(const std::vector<const char*>& extensions);
    };


    class Instance {
    private:
        VkInstance m_instance = {};

    public:
        Instance() = default;
        explicit Instance(const InstanceInfo& instanceInfo);
        ~Instance();

        khr::Surface createSurface(GLFWwindow* window);

        VkInstance vkInstance() const;
	};

}


