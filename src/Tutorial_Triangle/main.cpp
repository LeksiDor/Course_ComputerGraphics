#include "VulkanBase.h"
#include "VulkanContext.h"
#include "CommandPool.h"
#include "Image.h"
#include "SwapChain.h"

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


class HelloTriangleApplication {
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow( WIDTH, HEIGHT, TUTORIAL_NAME.c_str(), nullptr, nullptr );
    }

    void initVulkan()
    {
        auto& context = theVulkanContext();
        context.Init(
            TUTORIAL_NAME,
            window,
            { "VK_LAYER_KHRONOS_validation" },
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME }
            );
        commandPool.reset( new CommandPool( context.GraphicsFamily().value() ) );

        swapchain.reset( new SwapChain() );
        swapchain->commandPool = commandPool;
        swapchain->renderEntryManager = nullptr;
        swapchain->window = window;
        swapchain->vertShaderPath = std::string(PROJECT_NAME) + "/shader.vert.spv";
        swapchain->fragShaderPath = std::string(PROJECT_NAME) + "/shader.frag.spv";

        swapchain->swapChainInfo.Update();
        swapchain->createSwapChain();
        swapchain->createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
        swapchain->createSyncObjects();
    }

    void mainLoop()
    {
        const auto device = theVulkanContext().LogicalDevice();
        while ( !glfwWindowShouldClose(window) )
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle( device );
    }

    void cleanup()
    {
        swapchain.reset();
        commandPool.reset();
        theVulkanContext().Destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createRenderPass()
    {
        const auto device = theVulkanContext().LogicalDevice();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchain->swapChainInfo.imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &swapchain->renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline()
    {
        const auto device = theVulkanContext().LogicalDevice();

        auto vertShaderCode = LoadShaderCode( std::string(PROJECT_NAME) + "/shader.vert.spv" );
        auto fragShaderCode = LoadShaderCode( std::string(PROJECT_NAME) + "/shader.frag.spv" );

        VkShaderModule vertShaderModule = CreateShaderModule( device, vertShaderCode );
        VkShaderModule fragShaderModule = CreateShaderModule( device, fragShaderCode );

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchain->swapChainInfo.extent.width;
        viewport.height = (float) swapchain->swapChainInfo.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain->swapChainInfo.extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &swapchain->pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = swapchain->pipelineLayout;
        pipelineInfo.renderPass = swapchain->renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &swapchain->graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers()
    {
        const auto device = theVulkanContext().LogicalDevice();

        for ( auto& entry : swapchain->swapChainEntries )
        {
            VkImageView attachments[] = {
                entry.imageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = swapchain->renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapchain->swapChainInfo.extent.width;
            framebufferInfo.height = swapchain->swapChainInfo.extent.height;
            framebufferInfo.layers = 1;

            if ( vkCreateFramebuffer( device, &framebufferInfo, nullptr, &entry.framebuffer ) != VK_SUCCESS )
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    void createCommandBuffers()
    {
        for ( auto& entry : swapchain->swapChainEntries )
        {
            entry.commandBuffer = commandPool->CreateCommandBuffer();
            CommandPool::BeginCommandBuffer( entry.commandBuffer, false );

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = swapchain->renderPass;
            renderPassInfo.framebuffer = entry.framebuffer;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchain->swapChainInfo.extent;

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass( entry.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

            vkCmdBindPipeline( entry.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapchain->graphicsPipeline );

            vkCmdDraw( entry.commandBuffer, 3, 1, 0, 0 );

            vkCmdEndRenderPass( entry.commandBuffer );

            CommandPool::EndCommandBuffer( entry.commandBuffer );
        }
    }

    void drawFrame()
    {
        const auto device = theVulkanContext().LogicalDevice();
        const auto presentQueue = theVulkanContext().PresentQueue();
        const int maxFramesInFlight = theVulkanContext().MaxFramesInFlight();

        auto& fenceEntry = swapchain->fenceEntries[swapchain->currentFrame];

        vkWaitForFences( device, 1, &fenceEntry.inFlightFence, VK_TRUE, UINT64_MAX );

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR( device, swapchain->swapChain, UINT64_MAX, fenceEntry.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

        auto& swapChainEntry = swapchain->swapChainEntries[imageIndex];

        if ( swapChainEntry.imageInFlight != VK_NULL_HANDLE )
            vkWaitForFences( device, 1, &swapChainEntry.imageInFlight, VK_TRUE, UINT64_MAX );
        swapChainEntry.imageInFlight = fenceEntry.inFlightFence;

        const std::vector<VkSemaphore> waitSemaphores = { fenceEntry.imageAvailableSemaphore };
        const std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const std::vector<VkSemaphore> signalSemaphores = { fenceEntry.renderFinishedSemaphore };

        theVulkanContext().SubmitGraphicsQueue( swapChainEntry.commandBuffer, waitSemaphores, waitStages, signalSemaphores, fenceEntry.inFlightFence );

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();

        VkSwapchainKHR swapChains[] = {swapchain->swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        swapchain->currentFrame = (swapchain->currentFrame + 1) % theVulkanContext().MaxFramesInFlight();
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