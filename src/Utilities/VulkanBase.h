#ifndef VULKANBASE_H
#define VULKANBASE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <string>
#include <vector>


// Set up this struct globally before using Vulkan.
struct VulkanInfo
{
    // Level of Vulkan message severity to be displayed.
    // Default level: warning.
    VkDebugUtilsMessageSeverityFlagBitsEXT VulkanMessageLevelToDisplay;

    // Should be specified before Vulkan initialization.
    // Default value: 2.
    int MAX_FRAMES_IN_FLIGHT;

    bool enableValidationLayers;

    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions;
};


// Get global VulkanInfo.
const VulkanInfo& GetVulkanInfo();
// Set global VulkanInfo.
VulkanInfo& SetVulkanInfo();



VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger );

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator );


struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData );


std::vector<char> LoadShaderCode( const std::string& filename );


VkShaderModule CreateShaderModule( VkDevice device, const std::vector<char>& code );

VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );

VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );

VkExtent2D ChooseSwapExtent( GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities );

SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface );

bool IsDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface );

bool CheckDeviceExtensionSupport( VkPhysicalDevice device );

QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );

std::vector<const char*> GetRequiredExtensions();

bool CheckValidationLayerSupport();



#endif // VULKANBASE_H