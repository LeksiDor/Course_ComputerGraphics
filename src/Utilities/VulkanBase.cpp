#include "VulkanBase.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>


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


VkShaderModule CreateShaderModule( VkDevice device, const std::vector<char>& code )
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if ( vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
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
    return availableFormats[0];
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
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}


SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}


bool IsDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    QueueFamilyIndices indices = FindQueueFamilies( device, surface );

    bool extensionsSupported = CheckDeviceExtensionSupport( device );

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( device, surface );
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}


bool CheckDeviceExtensionSupport( VkPhysicalDevice device )
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions( GetVulkanInfo().deviceExtensions.begin(), GetVulkanInfo().deviceExtensions.end() );

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

        if ( presentSupport )
            indices.presentFamily = i;

        if ( indices.isComplete() )
            break;

        i++;
    }

    return indices;
}


std::vector<const char*> GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

    if ( GetVulkanInfo().enableValidationLayers )
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    return extensions;
}


bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for ( const char* layerName : GetVulkanInfo().validationLayers )
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if ( !layerFound )
            return false;
    }

    return true;
}