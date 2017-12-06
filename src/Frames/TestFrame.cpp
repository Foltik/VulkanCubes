#include "TestFrame.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>

#include "../Engine.h"

TestFrame::TestFrame(Engine& engine) : Frame(engine) {}

void TestFrame::update(float dt, Context& context) {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<float>(now - start).count();

    auto win = m_engine.window().window();
    if (glfwGetKey(win, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(win, true);

    if (glfwGetKey(win, GLFW_KEY_W))
        cameraPos += speed * cameraTarget;
    if (glfwGetKey(win, GLFW_KEY_S))
        cameraPos -= speed * cameraTarget;
    if (glfwGetKey(win, GLFW_KEY_D))
        cameraPos += speed * glm::normalize(glm::cross(cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (glfwGetKey(win, GLFW_KEY_A))
        cameraPos -= speed * glm::normalize(glm::cross(cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f)));

    static bool first = true;
    if (first) {
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);
        lastMouseX = static_cast<float>(mx);
        lastMouseY = static_cast<float>(my);
        first = false;
    }

    double mx;
    double my;
    glfwGetCursorPos(win, &mx, &my);
    auto mouseX = static_cast<float>(mx);
    auto mouseY = static_cast<float>(my);

    cameraAngles.x += (mouseX - lastMouseX) * sens;
    cameraAngles.y += (lastMouseY - mouseY) * sens;

    if (cameraAngles.x > 360.0f)
        cameraAngles.x -= 360.0f;
    if (cameraAngles.x < -360.0f)
        cameraAngles.x += 360.0f;
    if (cameraAngles.y < -89.0f)
        cameraAngles.y = -89.0f;
    if (cameraAngles.y > 89.0f)
        cameraAngles.y = 89.0f;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    cameraTarget.x = glm::cos(glm::radians(cameraAngles.y)) * glm::cos(glm::radians(cameraAngles.x));
    cameraTarget.y = glm::sin(glm::radians(cameraAngles.y));
    cameraTarget.z = glm::cos(glm::radians(cameraAngles.y)) * glm::sin(glm::radians(cameraAngles.x));

    //std::cout << cameraAngles.x << "," << cameraAngles.y << std::endl;

    mvp.view = glm::lookAt(cameraPos, cameraPos + cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    static float aspect = m_engine.window().width() / static_cast<float>(m_engine.window().height());
    mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    mvp.proj[1][1] *= -1;

    std::memcpy(context.m_vertexBufferMappedMem, verts.data(), verts.size() * sizeof(float));
    std::memcpy(context.m_indexBufferMappedMem, indices.data(), indices.size() * sizeof(uint16_t));

    float integral;
    std::modf(time, &integral);
    if (integral > lastIntegral) {
        std::cout << 1.0f / (frameTime / frameCount) << " FPS" << std::endl;
        frameTime = 0.0f;
        frameCount = 0;
        lastIntegral = integral;
    }
}

void TestFrame::render(Context& context) {
    auto start = std::chrono::steady_clock::now();

    // Acquire Image
    vkQueueWaitIdle(context.m_presentQueue);
    uint32_t imageIndex;
    vkAcquireNextImageKHR(context.m_device, context.m_swapChain, std::numeric_limits<uint64_t>::max(),
                          context.m_imageAvailable, VK_NULL_HANDLE,
                          &imageIndex);

    // Allocate command buffer
    VkCommandBuffer commandBuffer = {};
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(context.m_device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    // Record command buffer
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = context.m_renderPass;
    renderPassInfo.framebuffer = context.m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = context.m_swapChainExtent;
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.m_grahicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.m_pipelineLayout, 0, 1,
                            &context.m_descriptorSet, 0, nullptr);
    VkDeviceSize offset = 0;
    vkCmdPushConstants(commandBuffer, context.m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 48 * sizeof(float),
                       &mvp);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &context.m_vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, context.m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    // Submit command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {context.m_imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VkSemaphore signalSemaphores[] = {context.m_renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(context.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command queue!");

    // Present
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapChains[] = {context.m_swapChain};
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    vkQueuePresentKHR(context.m_presentQueue, &presentInfo);

    // Wait for idle and clean up
    vkDeviceWaitIdle(context.m_device);
    vkFreeCommandBuffers(context.m_device, context.m_commandPool, 1, &commandBuffer);

    auto end = std::chrono::steady_clock::now();
    frameTime += std::chrono::duration<float>(end - start).count();
    frameCount++;
}

void TestFrame::gen() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                if (i > 3 / 2 && i < 3 * 0.75 &&
                    j > 3 / 2 && j < 3 * 0.75 &&
                    k > 3 / 2 && k < 3 * 0.75) {
                    voxels[i][j][k] = {false, 1, 0};
                } else if (i == 0) {
                    voxels[i][j][k] = {false, 2, 0};
                } else {
                    voxels[i][j][k] = {false, 3, 0};
                }
            }
        }
    }
}

void TestFrame::mesh() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                if (!voxels[i][j][k].transparent) {
                    auto newCube = cubeVerts;
                    for (int l = 0; l < 8; l++) {
                        newCube[6 * l + 0] += i;
                        newCube[6 * l + 1] += j;
                        newCube[6 * l + 2] += k;
                    }

                    auto newInds = cubeIndices;
                        for (auto& index : cubeIndices)
                            index += 8;

                    verts.insert(verts.end(), newCube.begin(), newCube.end());
                    indices.insert(indices.end(), newInds.begin(), newInds.end());
                }
            }
        }
    }
}

void TestFrame::enter() {
    //glfwSetInputMode(m_engine.window().window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    static bool hasInitialized = false;
    if (!hasInitialized) {
        // Generate voxels
        gen();

        // Generate mesh
        mesh();
        hasInitialized = true;
    }
}

void TestFrame::leave() {

}
