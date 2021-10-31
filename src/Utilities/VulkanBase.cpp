#include "VulkanBase.h"
#include "VulkanContext.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>



namespace svk {


std::vector<char> LoadShaderCode( const std::string& filename )
{
    std::ifstream file( filename, std::ios::ate | std::ios::binary );

    if ( !file.is_open() ) {
        throw std::runtime_error( "failed to open file!" );
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data(), fileSize );

    file.close();

    return buffer;
}


VkShaderModule CreateShaderModule( VkDevice device, const std::vector<char>& code )
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );

    VkShaderModule shaderModule;
    if ( vkCreateShaderModule( device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create shader module!" );
    }

    return shaderModule;
}


VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
{
    for ( const auto& availableFormat : availableFormats )
    {
        if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            return availableFormat;
    }
    return availableFormats[ 0 ];
}


VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes )
{
    for ( const auto& availablePresentMode : availablePresentModes )
    {
        if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D ChooseSwapExtent( GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities )
{
    if ( capabilities.currentExtent.width != UINT32_MAX ) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );

        VkExtent2D actualExtent = {
            static_cast<uint32_t>( width ),
            static_cast<uint32_t>( height )
        };

        actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
        actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

        return actualExtent;
    }
}


SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 ) {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 ) {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
    }

    return details;
}


void createBuffer( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory )
{
    const auto device = theVulkanContext().LogicalDevice();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
        throw std::runtime_error( "failed to create buffer!" );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = theVulkanContext().FindMemoryType( memRequirements.memoryTypeBits, properties );

    if ( vkAllocateMemory( device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
        throw std::runtime_error("failed to allocate buffer memory!");

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}


} // namespace svk