#include "SwapChain.h"
#include "VulkanContext.h"
#include "VulkanBase.h"
#include "CommandPool.h"
#include "Image.h"

#include <stdexcept>
#include <array>


namespace svk {

void SwapChainInfo::Update()
{
    const auto window = theVulkanContext().Window();
    const auto surface = theVulkanContext().Surface();
    const auto physicalDevice = theVulkanContext().PhysicalDevice();

    const auto support = QuerySwapChainSupport( physicalDevice, surface );
    capabilities = support.capabilities;
    formats = support.formats;
    presentModes = support.presentModes;

    surfaceFormat = ChooseSwapSurfaceFormat( support.formats );
    imageFormat = surfaceFormat.format;
    presentMode = ChooseSwapPresentMode( support.presentModes );
    extent = ChooseSwapExtent( window, support.capabilities );

    numEntries = support.capabilities.minImageCount + 1;
    if ( support.capabilities.maxImageCount > 0 && numEntries > support.capabilities.maxImageCount )
        numEntries = support.capabilities.maxImageCount;
}


void SwapChain::Init(
    std::shared_ptr<CommandPool> commandPool,
    RenderEntryManager* renderEntryManager,
    const std::string& vertShaderPath,
    const std::string& fragShaderPath )
{
    this->commandPool = commandPool;
    this->renderEntryManager = renderEntryManager;
    this->window = theVulkanContext().Window();
    this->vertShaderPath = vertShaderPath;
    this->fragShaderPath = fragShaderPath;

    swapChainInfo.Update();
    renderEntryManager->InitRenderEntries( swapChainInfo );
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createDescriptorPool();
    createSwapChain();
    createImageViews();
    createFramebuffers();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}


SwapChain::~SwapChain()
{
    const auto device = theVulkanContext().LogicalDevice();
    cleanupSwapChain();
    if ( descriptorSetLayout != VK_NULL_HANDLE )
        vkDestroyDescriptorSetLayout( device, descriptorSetLayout, nullptr );
    for ( auto& entry : fenceEntries )
    {
        vkDestroySemaphore( device, entry.renderFinishedSemaphore, nullptr );
        vkDestroySemaphore( device, entry.imageAvailableSemaphore, nullptr );
        vkDestroyFence( device, entry.inFlightFence, nullptr );
    }
    fenceEntries.clear();
    depthImage.reset();
}


void SwapChain::DrawFrame()
{
    const auto device = theVulkanContext().LogicalDevice();
    const auto presentQueue = theVulkanContext().PresentQueue();
    const int maxFramesInFlight = theVulkanContext().MaxFramesInFlight();

    auto& fenceEntry = fenceEntries[currentFrame];

    vkWaitForFences( device, 1, &fenceEntry.inFlightFence, VK_TRUE, UINT64_MAX );

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, fenceEntry.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto& swapChainEntry = swapChainEntries[imageIndex];

    renderEntryManager->UpdateRenderEntry( swapChainInfo, swapChainEntry, imageIndex );

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

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}


void SwapChain::cleanupSwapChain()
{
    const auto device = theVulkanContext().LogicalDevice();

    depthImage.reset();

    for ( auto& entry : swapChainEntries )
    {
        vkDestroyFramebuffer( device, entry.framebuffer, nullptr );
        commandPool->FreeCommandBuffer( entry.commandBuffer );
        vkDestroyImageView( device, entry.imageView, nullptr );
    }
    swapChainEntries.clear();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    if ( renderEntryManager != nullptr )
        renderEntryManager->ClearRenderEntries();

    vkDestroyDescriptorPool( device, descriptorPool, nullptr );
}


void SwapChain::recreateSwapChain()
{
    const auto device = theVulkanContext().LogicalDevice();

    int width = 0, height = 0;
    glfwGetFramebufferSize( window, &width, &height );
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize( window, &width, &height );
        glfwWaitEvents();
    }

    vkDeviceWaitIdle( device );

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    renderEntryManager->InitRenderEntries( swapChainInfo );
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}


void SwapChain::createSwapChain()
{
    const auto device = theVulkanContext().LogicalDevice();
    const auto physicalDevice = theVulkanContext().PhysicalDevice();
    const auto surface = theVulkanContext().Surface();
    const auto graphicsFamily = theVulkanContext().GraphicsFamily();
    const auto presentFamily = theVulkanContext().PresentFamily();

    swapChainInfo.Update();

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = swapChainInfo.numEntries;
    createInfo.imageFormat = swapChainInfo.imageFormat;
    createInfo.imageColorSpace = swapChainInfo.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainInfo.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { graphicsFamily.value(), presentFamily.value() };

    if ( graphicsFamily != presentFamily ) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainInfo.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapChainInfo.presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
        throw std::runtime_error("failed to create swap chain!");

    vkGetSwapchainImagesKHR( device, swapChain, &swapChainInfo.numEntries, nullptr );

    swapChainEntries.resize( swapChainInfo.numEntries );

    std::vector<VkImage> swapChainImages( swapChainInfo.numEntries );
    vkGetSwapchainImagesKHR( device, swapChain, &swapChainInfo.numEntries, swapChainImages.data() );
    for ( int i = 0; i < swapChainInfo.numEntries; ++i )
        swapChainEntries[i].image = swapChainImages[i];
}


void SwapChain::createImageViews()
{
    for ( auto& entry : swapChainEntries )
        entry.imageView = Image::CreateImageView( entry.image, swapChainInfo.imageFormat );
}

void SwapChain::createRenderPass()
{
    const auto device = theVulkanContext().LogicalDevice();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainInfo.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if ( vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
        throw std::runtime_error("failed to create render pass!");
}

void SwapChain::createDescriptorSetLayout()
{
    const auto device = theVulkanContext().LogicalDevice();

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if ( vkCreateDescriptorSetLayout( device, &layoutInfo, nullptr, &descriptorSetLayout ) != VK_SUCCESS )
        throw std::runtime_error("failed to create descriptor set layout!");
}


void SwapChain::createGraphicsPipeline()
{
    const auto device = theVulkanContext().LogicalDevice();

    auto vertShaderCode = LoadShaderCode( vertShaderPath );
    auto fragShaderCode = LoadShaderCode( fragShaderPath );

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

    if ( renderEntryManager != nullptr )
    {
        auto bindingDescriptions = renderEntryManager->getVertexBindingDescriptions();
        auto attributeDescriptions = renderEntryManager->getVertexAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
        vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainInfo.extent.width;
    viewport.height = (float) swapChainInfo.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainInfo.extent;

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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

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
    if ( descriptorSetLayout != VK_NULL_HANDLE )
    {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    }
    else
    {
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = VK_NULL_HANDLE;
    }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void SwapChain::createFramebuffers()
{
    const auto device = theVulkanContext().LogicalDevice();

    for ( auto& entry : swapChainEntries )
    {
        const std::vector<VkImageView> attachments = {
            entry.imageView,
            depthImage->Info().imageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainInfo.extent.width;
        framebufferInfo.height = swapChainInfo.extent.height;
        framebufferInfo.layers = 1;

        if ( vkCreateFramebuffer( device, &framebufferInfo, nullptr, &entry.framebuffer ) != VK_SUCCESS )
            throw std::runtime_error("failed to create framebuffer!");
    }
}

void SwapChain::createDepthResources() {
    const VkFormat depthFormat = findDepthFormat();
    depthImage.reset( new Image( swapChainInfo.extent.width, swapChainInfo.extent.height, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT ) );
}


void SwapChain::createDescriptorPool()
{
    const auto device = theVulkanContext().LogicalDevice();

    const auto descriptorTypes = renderEntryManager->getDescriptorTypes();
    const uint32_t numDescriptorTypes = descriptorTypes.size();

    if ( numDescriptorTypes == 0 )
        return;

    std::vector<VkDescriptorPoolSize> poolSizes( numDescriptorTypes );
    for ( int i = 0; i < numDescriptorTypes; ++i )
    {
        poolSizes[i].type = descriptorTypes[i];
        poolSizes[i].descriptorCount = swapChainInfo.numEntries;
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = swapChainInfo.numEntries;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SwapChain::createDescriptorSets()
{
    if ( descriptorPool == VK_NULL_HANDLE )
        return;

    const auto device = theVulkanContext().LogicalDevice();

    std::vector<VkDescriptorSetLayout> layouts( swapChainInfo.numEntries, descriptorSetLayout );
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = swapChainInfo.numEntries;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets( swapChainInfo.numEntries );
    if ( vkAllocateDescriptorSets( device, &allocInfo, descriptorSets.data() ) != VK_SUCCESS )
        throw std::runtime_error( "failed to allocate descriptor sets!" );
    for ( int i = 0; i < swapChainInfo.numEntries; ++i )
        swapChainEntries[i].descriptorSet = descriptorSets[i];

    for ( int i = 0; i < swapChainInfo.numEntries; ++i )
    {
        const auto& swapChainEntry = swapChainEntries[i];
        const auto descriptorWrites = renderEntryManager->getDescriptorWrites( swapChainEntry.descriptorSet, i );
        vkUpdateDescriptorSets( device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr );
    }
}

void SwapChain::createCommandBuffers()
{
    for ( auto& entry : swapChainEntries )
    {
        entry.commandBuffer = commandPool->CreateCommandBuffer();
        CommandPool::BeginCommandBuffer( entry.commandBuffer, false );

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = entry.framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainInfo.extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass( entry.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

        vkCmdBindPipeline( entry.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );

        if ( entry.descriptorSet != VK_NULL_HANDLE )
            vkCmdBindDescriptorSets( entry.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &entry.descriptorSet, 0, nullptr );

        renderEntryManager->BindSwapEntryCommandBuffer( entry );

        vkCmdEndRenderPass( entry.commandBuffer );

        CommandPool::EndCommandBuffer( entry.commandBuffer );
    }
}

void SwapChain::createSyncObjects()
{
    const auto device = theVulkanContext().LogicalDevice();
    const int maxFramesInFlight = theVulkanContext().MaxFramesInFlight();

    fenceEntries.resize( maxFramesInFlight );

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for ( auto& entry : fenceEntries )
    {
        if (   vkCreateSemaphore( device, &semaphoreInfo, nullptr, &entry.imageAvailableSemaphore ) != VK_SUCCESS
            || vkCreateSemaphore( device, &semaphoreInfo, nullptr, &entry.renderFinishedSemaphore ) != VK_SUCCESS
            || vkCreateFence( device, &fenceInfo, nullptr, &entry.inFlightFence ) != VK_SUCCESS )
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}


VkFormat SwapChain::findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features )
{
    const auto physicalDevice = theVulkanContext().PhysicalDevice();

    for ( VkFormat format : candidates )
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if ( tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features )
            return format;
        else if ( tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features )
            return format;
    }

    throw std::runtime_error( "failed to find supported format!" );
}


VkFormat SwapChain::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}


} // namespace svk
