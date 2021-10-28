#ifndef SVK_VULKANCONTEXT_H
#define SVK_VULKANCONTEXT_H

#include <vulkan/vulkan.h>

#include <optional>
#include <string>
#include <vector>


struct GLFWwindow;


namespace svk {


class VulkanContext
{
public:

    // Constructors and desctuctor.

    VulkanContext() = default;

    void Init(
        const std::string& appName,
        GLFWwindow* window,
        const std::vector<const char*>& validationLayers,
        const std::vector<const char*>& deviceExtensions
    );

    void Destroy();


    // Member getters.

    VkDebugUtilsMessageSeverityFlagBitsEXT VulkanMessageLevelToDisplay() const { return vulkanMessageLevelToDisplay; }
    int MaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
    bool IsEnableValidationLayers() const { return enableValidationLayers; }

    const std::vector<const char*>& ValidationLayers() const { return validationLayers; }
    const std::vector<const char*>& DeviceExtensions() const { return deviceExtensions; }

    GLFWwindow* Window() const { return window; }
    VkInstance Instance() const { return instance; }
    VkSurfaceKHR Surface() const { return surface; }

    VkPhysicalDevice PhysicalDevice() const { return physicalDevice; }
    VkDevice LogicalDevice() const { return device; }

    const std::optional<uint32_t>& GraphicsFamily() const { return familyIndices.graphicsFamily; }
    const std::optional<uint32_t>& PresentFamily()  const { return familyIndices.presentFamily; }

    VkQueue GraphicsQueue() const { return graphicsQueue; }
    VkQueue PresentQueue()  const { return presentQueue; }


    // Create functions.

    VkCommandPool CreateCommandPool( const uint32_t familyIndex ) const;


    // Utility functions.

    uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const;

    void SubmitGraphicsQueue( const VkCommandBuffer& commandBuffer ) const;

    void SubmitGraphicsQueue(
        const VkCommandBuffer& commandBuffer,
        const std::vector<VkSemaphore>& waitSemaphores,
        const std::vector<VkPipelineStageFlags>& waitStages,
        const std::vector<VkSemaphore>& signalSemaphores,
        const VkFence fence = VK_NULL_HANDLE
    ) const;


private:
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


private:

    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    bool CheckValidationLayerSupport();

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();

    std::vector<const char*> GetRequiredExtensions();

    static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );

    static bool IsDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions );

    static SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface );

    static bool CheckDeviceExtensionSupport( VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions );


private:
    // Level of Vulkan message severity to be displayed.
    VkDebugUtilsMessageSeverityFlagBitsEXT vulkanMessageLevelToDisplay = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    // Should be specified before Vulkan initialization.
    int MAX_FRAMES_IN_FLIGHT = 2;

    bool enableValidationLayers = false;

    std::string appName;

    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions;

    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    QueueFamilyIndices familyIndices;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};


VulkanContext& theVulkanContext();


} // namespace svk

#endif // SVK_VULKANCONTEXT_H