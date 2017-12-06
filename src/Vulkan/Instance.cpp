#include "Instance.h"

#include "Surface.h"

using namespace vk;

void InstanceInfo::setLayers(const std::vector<const char*>& layers) {
    // Verify all layers are supported
    auto it = util::compareProps<VkLayerProperties>(layers, util::getSupportedLayers());
    if (it != layers.end())
        throw std::runtime_error("Layer " + std::string(*it) + " not found.");

    enabledLayerCount = static_cast<uint32_t>(layers.size());
    ppEnabledLayerNames = layers.data();
}

void InstanceInfo::setExtensions(const std::vector<const char*>& extensions) {
    // Verify all extensions are supported
    auto it = util::compareProps<VkExtensionProperties>(extensions, util::getSupportedExtensions());
    if (it != extensions.end())
        throw std::runtime_error("Extension " + std::string(*it) + " not found.");

    enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ppEnabledExtensionNames = extensions.data();
}



Instance::Instance(const InstanceInfo& instanceInfo) {
    if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

Instance::~Instance() {
    vkDestroyInstance(m_instance, nullptr);
}

VkInstance Instance::vkInstance() const {
    return m_instance;
}

khr::Surface Instance::createSurface(GLFWwindow* window) {
    return khr::Surface(*this, window);
}
