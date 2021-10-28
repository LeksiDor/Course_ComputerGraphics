#include "VulkanContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>



namespace svk {


VulkanContext& theVulkanContext()
{
    static VulkanContext context;
    return context;
}


void VulkanContext::Init(
    const std::string& appName,
    GLFWwindow* window,
    const std::vector<const char*>& validationLayers,
    const std::vector<const char*>& deviceExtensions )
{
    this->appName = appName;
    this->window = window;
    this->validationLayers = validationLayers;
    this->deviceExtensions = deviceExtensions;

#ifdef NDEBUG
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif

    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    familyIndices = FindQueueFamilies( physicalDevice, surface );
    CreateLogicalDevice();
}


void VulkanContext::Destroy()
{
    vkDestroyDevice( device, nullptr );

    if ( enableValidationLayers )
    {
        const VkAllocationCallbacks* pAllocator = nullptr;
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
        if ( func != nullptr ) {
            func( instance, debugMessenger, pAllocator );
        }
    }

    vkDestroySurfaceKHR( instance, surface, nullptr );
    vkDestroyInstance( instance, nullptr );

    window = nullptr;
    instance = VK_NULL_HANDLE;
    debugMessenger = VK_NULL_HANDLE;
    surface = VK_NULL_HANDLE;
    physicalDevice = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    familyIndices = {};
    graphicsQueue = VK_NULL_HANDLE;
    presentQueue = VK_NULL_HANDLE;
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    if ( messageSeverity >= theVulkanContext().VulkanMessageLevelToDisplay() )
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}


uint32_t VulkanContext::FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
    {
        if ( (typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties )
            return i;
    }

    throw std::runtime_error( "failed to find suitable memory type!" );
}


void VulkanContext::SubmitGraphicsQueue( const VkCommandBuffer& commandBuffer ) const
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if ( vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) != VK_SUCCESS )
        throw std::runtime_error( "Failed to submit graphics queue." );
    vkQueueWaitIdle( graphicsQueue );
}


void VulkanContext::SubmitGraphicsQueue( const VkCommandBuffer& commandBuffer, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkPipelineStageFlags>& waitStages, const std::vector<VkSemaphore>& signalSemaphores, const VkFence fence ) const
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    if ( waitSemaphores.size() != waitStages.size() )
        throw std::runtime_error( "Number of wait semaphores and wait stages must be the same." );

    if ( waitSemaphores.size() != 0 )
    {
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if ( signalSemaphores.size() != 0 )
    {
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();
    }

    vkResetFences( device, 1, &fence );

    if ( vkQueueSubmit( graphicsQueue, 1, &submitInfo, fence ) != VK_SUCCESS )
        throw std::runtime_error( "Failed to submit graphics queue." );
    vkQueueWaitIdle( graphicsQueue );
}


void VulkanContext::CreateInstance()
{
    if ( enableValidationLayers && !CheckValidationLayerSupport() )
        throw std::runtime_error( "validation layers requested, but not available!" );

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
    createInfo.ppEnabledExtensionNames = extensions.data();

    if ( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();
        const auto debugCreateInfo = CreateDebugMessengerCreateInfo();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if ( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS )
        throw std::runtime_error( "failed to create instance!" );
}


void VulkanContext::SetupDebugMessenger()
{
    if ( !enableValidationLayers )
        return;

    const auto createInfo = CreateDebugMessengerCreateInfo();

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func == nullptr || func( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
        throw std::runtime_error( "failed to set up debug messenger!" );
}


void VulkanContext::CreateSurface()
{
    if ( glfwCreateWindowSurface( instance, window, nullptr, &surface ) != VK_SUCCESS )
        throw std::runtime_error( "failed to create window surface!" );
}


void VulkanContext::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

    if ( deviceCount == 0 )
        throw std::runtime_error( "failed to find GPUs with Vulkan support!" );

    std::vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

    for ( const auto& device : devices )
    {
        if ( IsDeviceSuitable( device, surface, deviceExtensions ) )
        {
            physicalDevice = device;
            break;
        }
    }

    if ( physicalDevice == VK_NULL_HANDLE )
        throw std::runtime_error( "failed to find a suitable GPU!" );
}


void VulkanContext::CreateLogicalDevice()
{
    QueueFamilyIndices indices = familyIndices;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back( queueCreateInfo );
    }

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if ( enableValidationLayers ) {
        createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create logical device!" );
    }

    vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
    vkGetDeviceQueue( device, indices.presentFamily.value(), 0, &presentQueue );
}


bool VulkanContext::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    for ( const char* layerName : validationLayers )
    {
        bool layerFound = false;

        for ( const auto& layerProperties : availableLayers )
        {
            if ( strcmp( layerName, layerProperties.layerName ) == 0 )
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


VkDebugUtilsMessengerCreateInfoEXT VulkanContext::CreateDebugMessengerCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VulkanDebugCallback;
    return createInfo;
}


std::vector<const char*> VulkanContext::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

    if ( enableValidationLayers )
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    return extensions;
}


VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    int i = 0;
    for ( const auto& queueFamily : queueFamilies )
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


bool VulkanContext::IsDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions )
{
    QueueFamilyIndices indices = FindQueueFamilies( device, surface );

    bool extensionsSupported = CheckDeviceExtensionSupport( device, deviceExtensions );

    bool swapChainAdequate = false;
    if ( extensionsSupported )
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( device, surface );
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures( device, &supportedFeatures );

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}


VulkanContext::SwapChainSupportDetails VulkanContext::QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 )
    {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 )
    {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
    }

    return details;
}


bool VulkanContext::CheckDeviceExtensionSupport( VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions )
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

    std::vector<VkExtensionProperties> availableExtensions( extensionCount );
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

    std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

    for ( const auto& extension : availableExtensions )
        requiredExtensions.erase( extension.extensionName );

    return requiredExtensions.empty();
}


} // namespace svk