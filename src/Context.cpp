#include "Context.h"

#include <iostream>
#include <fstream>
#include <set>
#include <chrono>

#include "Window.h"
#include "Shader/Shader.h"
#include "Threads/Scheduler.h"

namespace {
    VkResult CreateDebugReportCallbackEXT(VkInstance instance,
                                          const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugReportCallbackEXT* pCallback) {
        auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance,
                                                                              "vkCreateDebugReportCallbackEXT");
        return func != nullptr ? func(instance, pCreateInfo, pAllocator, pCallback) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance,
                                                                               "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr) func(instance, callback, pAllocator);
    }
}

VkBool32 Context::debugCallback(VkDebugReportFlagsEXT flags,
                                VkDebugReportObjectTypeEXT objType,
                                uint64_t obj, size_t location,
                                int32_t code,
                                const char* layerPrefix,
                                const char* msg,
                                void* userData) {
    std::cerr << "Validation Layer: " << msg << std::endl;
    return VK_FALSE;
}

void Context::init(Window& window, bool debug) {
    // Create the Vulkan instance
    createInstance(debug);

    // Create the presentation surface for GLFW
    createSurface(window);

    // Set the callback for the Debug Validation Layers if needed
    if (debug)
        createCallback();

    selectPhysicalDevice();
    createLogicalDevice(debug);
    createSwapChain(window);
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createSemaphores();
}

Context::Context(Window& window, bool debug) {
    init(window, debug);
}

Context::~Context() {
    vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
    vkDestroySemaphore(m_device, m_renderFinished, nullptr);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto& framebuffer : m_swapChainFramebuffers)
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);

    vkDestroyPipeline(m_device, m_grahicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (auto& imageView : m_swapChainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    vkUnmapMemory(m_device, m_vertexBufferMem);
    m_vertexBufferMappedMem = nullptr;
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMem, nullptr);

    vkUnmapMemory(m_device, m_indexBufferMem);
    m_indexBufferMappedMem = nullptr;
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMem, nullptr);

    vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
    vkFreeMemory(m_device, m_uniformBufferMem, nullptr);

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    DestroyDebugReportCallbackEXT(m_instance, m_debugReportCallback, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void Context::createInstance(bool debug) {
    vk::ApplicationInfo appInfo("OpenMiner");
    vk::InstanceInfo instanceInfo(appInfo);

    // Debug validation layers
    std::vector<const char*> extensions;
    if (debug) {
        instanceInfo.setLayers(m_validationLayers);
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // GLFW Extensions
    unsigned int glfwExtCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    for (auto i = 0; i < glfwExtCount; i++)
        extensions.push_back(glfwExtensions[i]);
    instanceInfo.setExtensions(extensions);

    if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Vulkan Instance!");
}

void Context::createSurface(Window& window) {
    if (glfwCreateWindowSurface(m_instance, window.window(), nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void Context::createCallback() {
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    if (CreateDebugReportCallbackEXT(m_instance, &createInfo, nullptr, &m_debugReportCallback) != VK_SUCCESS)
        throw std::runtime_error("Failed to create debug report callback!");
}

void Context::selectPhysicalDevice() {
    auto devices = getDevices();
    if (devices.empty())
        throw std::runtime_error("Failed to find any compatible GPUs!");

    // Since Multimap is sorted, rbegin will be the device with the highest score.
    m_physicalDevice = devices.rbegin()->second;
}

void Context::createLogicalDevice(bool debug) {
    QueueFamilyIndices indices = getQueueFamilyIndices(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphics, indices.present};
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo = {};
    VkPhysicalDeviceFeatures deviceFeatures = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledLayerCount = 0;
    if (debug) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.graphics), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.present), 0, &m_presentQueue);
}

void Context::createSwapChain(Window& window) {
    SwapChainSupportInfo swapChainSupport = getSwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = selectSwapChainSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = selectSwapChainPresentMode(swapChainSupport.modes);
    VkExtent2D extent = selectSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = getQueueFamilyIndices(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(indices.graphics), static_cast<uint32_t>(indices.present)};
    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainImageCount, nullptr);
    m_swapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainImageCount, m_swapChainImages.data());

    m_swapChainFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void Context::createImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views!");
    }
}

void Context::createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass!");
}

void Context::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayout = {};
    uboLayout.binding = 0;
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayout.descriptorCount = 1;
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayout.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayout;

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void Context::createPipeline() {
    Shader vert(m_device, "Shaders/vert.spv");
    Shader frag(m_device, "Shaders/frag.spv");

    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vert.module();
    vertStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = frag.module();
    fragStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = 6 * sizeof(float);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {0};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 3 * sizeof(float);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.lineWidth = 1.0f;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateInfo.depthBiasClamp = 0.0f;
    rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
    multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.minSampleShading = 1.0f;
    multisampleStateInfo.pSampleMask = nullptr;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendState = {};
    colorBlendState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendState.blendEnable = VK_TRUE;
    colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendState;
    colorBlendStateInfo.blendConstants[0] = 0.0f;
    colorBlendStateInfo.blendConstants[1] = 0.0f;
    colorBlendStateInfo.blendConstants[2] = 0.0f;
    colorBlendStateInfo.blendConstants[3] = 0.0f;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 48 * sizeof(float);

    VkPipelineLayoutCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineCreateInfo.setLayoutCount = 1;
    pipelineCreateInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineCreateInfo.pushConstantRangeCount = 1;
    pipelineCreateInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device, &pipelineCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateInfo;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_grahicsPipeline) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");
}

void Context::createFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void Context::createCommandPool() {
    QueueFamilyIndices indices = getQueueFamilyIndices(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(indices.graphics);
    poolInfo.flags = 0;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
}


void Context::createVertexBuffer() {
    createBuffer(m_vertexSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_vertexBuffer,
                 m_vertexBufferMem);

    vkMapMemory(m_device, m_vertexBufferMem, 0, m_vertexSize, 0, &m_vertexBufferMappedMem);
    /*
    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMem = {};
    createBuffer(sizeof(verts),
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMem);


    void* data;
    vkMapMemory(m_device, stagingBufferMem, 0, sizeof(verts), 0, &data);
    std::memcpy(data, verts, sizeof(verts));
    vkUnmapMemory(m_device, stagingBufferMem);

    createBuffer(sizeof(verts),
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_vertexBuffer,
                 m_vertexBufferMem);

    copyBuffer(stagingBuffer, m_vertexBuffer, sizeof(verts));

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMem, nullptr);
     */
}

void Context::createIndexBuffer() {
    createBuffer(m_indexSize,
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_indexBuffer,
                 m_indexBufferMem);

    vkMapMemory(m_device, m_indexBufferMem, 0, m_indexSize, 0, &m_indexBufferMappedMem);
}

void Context::createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void Context::createDescriptorSet() {
    VkDescriptorSetLayout layouts[] = {m_descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = m_uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(MVP);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

/*
 * No longer using baked command buffers
 *
void Context::createCommandBuffers() {
    m_commandBuffers.resize(m_swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");

    for (size_t i = 0; i < m_commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_grahicsPipeline);

        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &m_vertexBuffer, &offset);
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        //vkCmdDraw(m_commandBuffers[i], 6, 1, 0, 0);
        vkCmdDrawIndexed(m_commandBuffers[i], 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }
}
*/

void Context::createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(MVP);
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_uniformBuffer,
                 m_uniformBufferMem);
}

void Context::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinished) != VK_SUCCESS)
        throw std::runtime_error("Failed to create semaphores!");
}


std::vector<VkLayerProperties> Context::getSupportedLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return availableLayers;
}

std::vector<VkExtensionProperties> Context::getSupportedExtensions() {
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    return availableExtensions;
}

std::vector<VkExtensionProperties> Context::getSupportedDeviceExtensions(VkPhysicalDevice& device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    return availableExtensions;
}

bool Context::supportsLayers(const std::vector<const char*>& layerNames) {
    auto supportedLayers = getSupportedLayers();
    for (const auto* layerName : layerNames) {
        bool found = false;
        for (const auto& props : supportedLayers) {
            if (strcmp(layerName, props.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool Context::supportsExtensions(const std::vector<const char*>& extensionNames) {
    auto supportedExtensions = getSupportedExtensions();
    for (const auto* extensionName : extensionNames) {
        bool found = false;
        for (const auto& props : supportedExtensions) {
            if (strcmp(extensionName, props.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool Context::supportsDeviceExtensions(VkPhysicalDevice& device, const std::vector<const char*>& extensionNames) {
    auto supportedExtensions = getSupportedDeviceExtensions(device);
    for (const auto* extensionName : extensionNames) {
        bool found = false;
        for (const auto& props : supportedExtensions) {
            if (strcmp(extensionName, props.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

std::multimap<int, VkPhysicalDevice> Context::getDevices() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> scoredDevices;
    for (auto& device : devices)
        if (deviceCompatible(device))
            scoredDevices.insert(std::make_pair(scoreDevice(device), device));

    return scoredDevices;
}

std::vector<VkQueueFamilyProperties> Context::getQueueFamilyProperties(VkPhysicalDevice& device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

    return queueFamilyProperties;
}

Context::QueueFamilyIndices Context::getQueueFamilyIndices(VkPhysicalDevice& device) {
    QueueFamilyIndices indices;

    auto queueFamilyProps = getQueueFamilyProperties(device);
    for (int i = 0; i < queueFamilyProps.size(); i++) {
        if (queueFamilyProps[i].queueCount > 0 && queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(i), m_surface, &presentSupport);
        if (presentSupport)
            indices.present = i;

        if (indices.complete())
            break;
    }

    return indices;
}

Context::SwapChainSupportInfo Context::getSwapChainSupport(VkPhysicalDevice& device) {
    SwapChainSupportInfo info = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &info.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        info.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, info.formats.data());
    }

    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount, nullptr);
    if (modeCount != 0) {
        info.modes.resize(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount, info.modes.data());
    }

    return info;
}

VkSurfaceFormatKHR Context::selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    for (const auto& format : formats)
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;

    return formats[0];
}

VkPresentModeKHR Context::selectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& modes) {
    auto bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& mode : modes)
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
        else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            bestMode = mode;

    return bestMode;
}

VkExtent2D Context::selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window& window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        VkExtent2D actualExtent = {static_cast<uint32_t>(window.width()), static_cast<uint32_t>(window.height())};
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

bool Context::deviceCompatible(VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties deviceProperties = {};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures = {};
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = getQueueFamilyIndices(device);

    bool extensionsSupported = supportsDeviceExtensions(device, m_deviceExtensions);
    SwapChainSupportInfo swapChainInfo = {};
    if (extensionsSupported) {
        swapChainInfo = getSwapChainSupport(device);
    }

    return indices.complete() && extensionsSupported && swapChainInfo.complete();
}

int Context::scoreDevice(VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(device, &properties);

    int score = 0;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    score += properties.limits.maxImageDimension2D;
    score += properties.limits.maxFramebufferWidth;
    score += properties.limits.maxFramebufferWidth;
    score += properties.limits.maxComputeSharedMemorySize;

    return score;
}

uint32_t Context::findMemType(uint32_t filter, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &deviceMemoryProperties);

    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
        if (filter & (1 << i) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;

    throw std::runtime_error("Failed to find memory type!");
}

void Context::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, VkBuffer& buffer,
                           VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer!");

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements(m_device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemType(memReqs.memoryTypeBits, flags);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate vertex buffer!");

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void Context::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffer!");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyInfo = {};
    copyInfo.srcOffset = 0;
    copyInfo.dstOffset = 0;
    copyInfo.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyInfo);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}
