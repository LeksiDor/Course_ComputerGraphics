#include "VulkanBase.h"

#include <iostream>
#include <fstream>


const VulkanInfo& GetVulkanInfo()
{
    return SetVulkanInfo();
}


VulkanInfo& SetVulkanInfo()
{
    // Static object VulkanInfo.
    static VulkanInfo theVulkanInfo = {
        // VulkanMessageLevelToDisplay
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        // MAX_FRAMES_IN_FLIGHT
        2,
        // enableValidationLayers
#ifdef NDEBUG
        false,
#else
        true,
#endif
        // validationLayers
        { "VK_LAYER_KHRONOS_validation" },
        // deviceExtensions
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
    };

    return theVulkanInfo;
}


VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator )
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
    if ( messageSeverity >= GetVulkanInfo().VulkanMessageLevelToDisplay )
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}


std::vector<char> LoadShaderCode( const std::string& filename )
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
