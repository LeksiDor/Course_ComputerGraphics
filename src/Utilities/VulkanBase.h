#ifndef SVK_VULKANBASE_H
#define SVK_VULKANBASE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <string>
#include <vector>



namespace svk {


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};




std::vector<char> LoadShaderCode( const std::string& filename );


VkShaderModule CreateShaderModule( VkDevice device, const std::vector<char>& code );

VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );

VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );

VkExtent2D ChooseSwapExtent( GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities );

SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface );


void createBuffer( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory );



} // namespace svk

#endif // SVK_VULKANBASE_H