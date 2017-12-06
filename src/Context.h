#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <map>

#include "Vulkan/Instance.h"
#include "Vulkan/Surface.h"

class Window;

class Context {
public:
    explicit Context(Window& window, bool debug);
    ~Context();

private:
    void init(Window& window, bool debug);

    void createInstance(bool debug);
    void createSurface(Window& window);
    void createCallback();
    void selectPhysicalDevice();
    void createLogicalDevice(bool debug);
    void createSwapChain(Window& window);
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();
    void createSemaphores();

    static std::vector<VkLayerProperties> getSupportedLayers();
    static std::vector<VkExtensionProperties> getSupportedExtensions();
    static std::vector<VkExtensionProperties> getSupportedDeviceExtensions(VkPhysicalDevice& device);

    static bool supportsLayers(const std::vector<const char*>& layerNames);
    static bool supportsExtensions(const std::vector<const char*>& extensionNames);
    static bool supportsDeviceExtensions(VkPhysicalDevice& device, const std::vector<const char*>& extensionNames);

    struct QueueFamilyIndices {
        int graphics = -1;
        int present = -1;
        inline bool complete() { return graphics >= 0 && present >= 0; }
    };
    std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice& device);
    QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice& device);

    std::multimap<int, VkPhysicalDevice> getDevices();
    bool deviceCompatible(VkPhysicalDevice& device);
    static int scoreDevice(VkPhysicalDevice& device);

    struct SwapChainSupportInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> modes;
        inline bool complete() { return !formats.empty() && !modes.empty(); }
    };
    SwapChainSupportInfo getSwapChainSupport(VkPhysicalDevice& device);
    VkSurfaceFormatKHR selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR selectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window& window);

    uint32_t findMemType(uint32_t filter, VkMemoryPropertyFlags flags);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* layerPrefix,
        const char* msg,
        void* userData);

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    int m_numInds;

    struct MVP {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    VkDebugReportCallbackEXT m_debugReportCallback;
    VkSurfaceKHR m_surface;
    vk::khr::Surface m_vsurface;

    vk::Instance m_vinstance;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkFormat m_swapChainFormat;
    VkExtent2D m_swapChainExtent;

    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipeline m_grahicsPipeline;
    VkPipelineLayout m_pipelineLayout;
    VkRenderPass m_renderPass;

    VkCommandPool m_commandPool;

    VkSemaphore m_imageAvailable;
    VkSemaphore m_renderFinished;

    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    static constexpr uint32_t m_vertexSize = 10240;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMem;
    void* m_vertexBufferMappedMem;

    static constexpr uint32_t m_indexSize = 10240;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMem;
    void* m_indexBufferMappedMem;

    VkBuffer m_uniformBuffer;
    VkDeviceMemory m_uniformBufferMem;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
};


