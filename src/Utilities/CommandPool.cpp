#include "CommandPool.h"
#include "VulkanContext.h"
#include <stdexcept>


namespace svk {



CommandPool::CommandPool( const uint32_t familyIndex )
{
    Reset( familyIndex );
}


CommandPool::~CommandPool()
{
    Clear();
}


void CommandPool::Reset( const uint32_t familyIndex )
{
    const auto device = theVulkanContext().LogicalDevice();
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndex;
    if ( vkCreateCommandPool( device, &poolInfo, nullptr, &commandPool ) != VK_SUCCESS )
        throw std::runtime_error( "failed to create command pool!" );
}


void CommandPool::Clear()
{
    const auto device = theVulkanContext().LogicalDevice();
    if ( commandPool != VK_NULL_HANDLE )
        vkDestroyCommandPool( device, commandPool, nullptr );
    commandPool = VK_NULL_HANDLE;
}


VkCommandBuffer CommandPool::CreateCommandBuffer() const
{
    const auto device = theVulkanContext().LogicalDevice();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if ( vkAllocateCommandBuffers( device, &allocInfo, &commandBuffer ) != VK_SUCCESS )
        throw std::runtime_error( "CommandPool: Failed to allocate command buffers." );
    return commandBuffer;
}


void CommandPool::FreeCommandBuffer( VkCommandBuffer& commandBuffer ) const
{
    const auto device = theVulkanContext().LogicalDevice();
    if ( commandBuffer != VK_NULL_HANDLE )
        vkFreeCommandBuffers( device, commandPool, 1, &commandBuffer );
    commandBuffer = VK_NULL_HANDLE;
}


void CommandPool::BeginCommandBuffer( const VkCommandBuffer commandBuffer, const bool isSingleUse )
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if ( isSingleUse )
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if ( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS )
        throw std::runtime_error( "CommandPool: Failed to start command buffer." );
}


void CommandPool::EndCommandBuffer( const VkCommandBuffer commandBuffer )
{
    if ( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
        throw std::runtime_error( "CommandPool: Failed to end command buffer." );
}

} // namespace svk