#include "Shader.h"

#include <vector>
#include <fstream>

namespace {
    std::vector<char> readFile(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open file!");

        auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }
}

Shader::Shader(VkDevice& device, const std::string& fileName) {
    m_device = device;

    auto code = readFile(fileName);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &m_module) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");
}

Shader::~Shader() {
    vkDestroyShaderModule(m_device, m_module, nullptr);
}

VkShaderModule Shader::module() {
    return m_module;
}


