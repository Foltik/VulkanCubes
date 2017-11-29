#include "Instance.h"

#include <iostream>
#include <cstring>

void vk::InstanceInfo::setLayers(const std::vector<const char*>& layers) {
    auto supported = Instance::getSupportedLayers();
    for (const auto* name : layers) {
        bool found = false;
        for (const auto& prop : supported) {
            if (strcmp(name, prop.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error("Layer " + std::string(name) + " not found.");
    }

    enabledLayerCount = static_cast<uint32_t>(layers.size());
    ppEnabledLayerNames = layers.data();
}

void vk::InstanceInfo::setExtensions(const std::vector<const char*>& extensions) {
    auto supported = Instance::getSupportedExtensions();
    for (const auto* name : extensions) {
        bool found = false;
        for (const auto& prop : supported) {
            if (strcmp(name, prop.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error("Extension " + std::string(name) + " not found.");
    }

    enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ppEnabledExtensionNames = extensions.data();
}

vk::Instance::Instance(const vk::InstanceInfo& instanceInfo) {
    if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

vk::Instance::~Instance() {
    vkDestroyInstance(m_instance, nullptr);
}

std::vector<VkExtensionProperties> vk::Instance::getSupportedExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    return extensions;
}

std::vector<VkLayerProperties> vk::Instance::getSupportedLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    return layers;
}
