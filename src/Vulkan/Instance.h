#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

#include "Util.h"
#include "VkTraits.h"
#include "Structure.h"
#include "Surface.h"

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

        void setLayers(const std::vector<const char*>& layers) {
            // Verify all layers are supported
            auto it = util::compareProps<VkLayerProperties>(layers, util::getSupportedLayers());
            if (it != layers.end())
                throw std::runtime_error("Layer " + std::string(*it) + " not found.");

            enabledLayerCount = static_cast<uint32_t>(layers.size());
            ppEnabledLayerNames = layers.data();
        };

        void setExtensions(const std::vector<const char*>& extensions) {
            // Verify all extensions are supported
            auto it = util::compareProps<VkExtensionProperties>(extensions, util::getSupportedExtensions());
            if (it != extensions.end())
                throw std::runtime_error("Extension " + std::string(*it) + " not found.");

            enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            ppEnabledExtensionNames = extensions.data();
        }
    };


    class Instance : public VkTraits<Instance, VkInstanceCreateInfo> {
    private:
        VkInstance m_instance = {};

    public:
        explicit Instance(const InstanceInfo& instanceInfo) {
            if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
                throw std::runtime_error("Failed to create instance!");
        }

        ~Instance() {
            vkDestroyInstance(m_instance, nullptr);
        }

        khr::Surface createSurface(GLFWwindow* window) {
            return khr::Surface(*this, window);
        }

        const VkInstance vkInstance() const {
            return m_instance;
        }
    };

}


