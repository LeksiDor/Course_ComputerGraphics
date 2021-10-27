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


} // namespace svk