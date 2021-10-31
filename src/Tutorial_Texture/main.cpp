#include "VulkanBase.h"
#include "VulkanContext.h"
#include "CommandPool.h"
#include "Image.h"
#include "SwapChain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string TUTORIAL_NAME = "Texture Mapping";

const std::string TEXTURE_PATH = std::string(ROOT_DIRECTORY) + "/media/texture.jpg";

using namespace svk;


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};


struct RenderEntry
{
    // Per-frame items.
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo bufferInfo{};
};


class HelloTriangleApplication : public RenderEntryManager
{
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    std::shared_ptr<SwapChain> swapchain;
    std::vector<RenderEntry> renderEntries;

    std::shared_ptr<CommandPool> commandPool;
    std::shared_ptr<Image> texture;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;


    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow( WIDTH, HEIGHT, TUTORIAL_NAME.c_str(), nullptr, nullptr );
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->swapchain->SetResizeFlag();
    }


public:
    virtual VkVertexInputBindingDescription getVertexBindingDescription() const override
    {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }

    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const override
    {
        return {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) },
        };
    }

    virtual std::vector<VkDescriptorType> getDescriptorTypes() const override
    {
        return { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
    }

    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const override
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites(2);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &renderEntries[swapEntryIndex].bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &texture->Info();

        return descriptorWrites;
    }

    virtual void InitRenderEntries( const SwapChainInfo& swapChainInfo ) override
    {
        ClearRenderEntries();
        renderEntries.resize( swapChainInfo.numEntries );
        const VkDeviceSize bufferSize = sizeof( UniformBufferObject );
        for ( int i = 0; i < swapChainInfo.numEntries; ++i )
        {
            auto& entry = renderEntries[i];
            createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, entry.uniformBuffer, entry.uniformBufferMemory );
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = entry.uniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            entry.bufferInfo = bufferInfo;
        }
    }

    virtual void ClearRenderEntries() override
    {
        const auto device = theVulkanContext().LogicalDevice();
        for ( auto& entry : renderEntries )
        {
            vkDestroyBuffer( device, entry.uniformBuffer, nullptr );
            vkFreeMemory( device, entry.uniformBufferMemory, nullptr );
        }
        renderEntries.clear();
    }

    virtual void UpdateRenderEntry( const SwapChainInfo& swapChainInfo, const SwapChainEntry& swapChainEntry, const int swapEntryIndex ) override
    {
        const auto device = theVulkanContext().LogicalDevice();
        auto& entry = renderEntries[swapEntryIndex];

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainInfo.extent.width / (float) swapChainInfo.extent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory( device, entry.uniformBufferMemory, 0, sizeof(ubo), 0, &data );
        memcpy( data, &ubo, sizeof(ubo) );
        vkUnmapMemory( device, entry.uniformBufferMemory );
    }

    virtual void ExecuteCmdDraw( const SwapChainEntry& entry ) override
    {
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers( entry.commandBuffer, 0, 1, vertexBuffers, offsets );
        vkCmdBindIndexBuffer( entry.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16 );
        vkCmdDrawIndexed( entry.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0 );
    }

    void initVulkan() {
        auto& context = theVulkanContext();
        context.Init(
            TUTORIAL_NAME,
            window,
            { "VK_LAYER_KHRONOS_validation" },
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME }
        );
        commandPool.reset( new CommandPool( context.GraphicsFamily().value() ) );

        // App-specific data.
        texture = Image::CreateFromFile( *commandPool, TEXTURE_PATH );
        createVertexBuffer();
        createIndexBuffer();

        // Swapchain.
        swapchain.reset( new SwapChain() );
        swapchain->Init(
            commandPool,
            this,
            std::string(PROJECT_NAME) + "/shader.vert.spv",
            std::string(PROJECT_NAME) + "/shader.frag.spv"
        );
    }

    void mainLoop()
    {
        const auto device = theVulkanContext().LogicalDevice();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            swapchain->DrawFrame();
        }
        vkDeviceWaitIdle(device);
    }

    void cleanup()
    {
        const auto device = theVulkanContext().LogicalDevice();

        swapchain.reset();

        texture.reset();

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        commandPool.reset();

        theVulkanContext().Destroy();

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createVertexBuffer()
    {
        const auto device = theVulkanContext().LogicalDevice();

        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        copyBuffer( *commandPool, stagingBuffer, vertexBuffer, bufferSize );

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {
        const auto device = theVulkanContext().LogicalDevice();

        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer( *commandPool, stagingBuffer, indexBuffer, bufferSize );

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
