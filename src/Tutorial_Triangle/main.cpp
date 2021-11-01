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
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string TUTORIAL_NAME = "Drawing a Triangle";

using namespace svk;


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};


const std::vector<Vertex> vertices = {
    { {  0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f} },
    { {  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f} },
    { { -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f} },
};

const std::vector<uint32_t> indices = { 0, 2, 1 };


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
    std::shared_ptr<CommandPool> commandPool;

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
        };
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

        // Swapchain.
        swapchain.reset( new SwapChain() );
        swapchain->Init(
            commandPool,
            this,
            vertices,
            indices,
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
        swapchain.reset();
        commandPool.reset();
        theVulkanContext().Destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
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