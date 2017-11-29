#pragma once

#include <vector>
#include <algorithm>

#include <vulkan/vulkan.h>

namespace vk::util {
    static std::vector<VkExtensionProperties> getSupportedExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        return extensions;
    };

    static std::vector<VkLayerProperties> getSupportedLayers() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        return layers;
    }

    template <typename T>
    const char* propName(const T& prop);
    template<>
    inline const char* propName<VkLayerProperties>(const VkLayerProperties& prop) {
        return prop.layerName;
    }
    template <>
    inline const char* propName<VkExtensionProperties>(const VkExtensionProperties& prop) {
        return prop.extensionName;
    }

    template <typename T>
    static auto compareProps(const std::vector<const char*>& names, const std::vector<T>& props) {
        for (auto it = names.begin(); it != names.end(); it++) {
            if (std::find_if(props.begin(), props.end(), [&](const T& prop) {
                return strcmp(propName<T>(prop), *it) == 0;
            }) == props.end())
                return it;
        }
        return names.end();
    }
}
