#pragma once

#include <vulkan/vulkan.h>

#include <string>

class Shader {
public:
    Shader() = default;
    Shader(VkDevice& device, const std::string& fileName);
    ~Shader();

    VkShaderModule module();

private:

    VkDevice m_device;
    VkShaderModule m_module;
};


