#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <string>
#include <vector>



class Application
{
public:

    using VectorCString = std::vector<const char*>;

    Application(
        const uint32_t width,
        const uint32_t height,
        const std::string& appName,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath,
        const VectorCString& validationLayers,
        const VectorCString& deviceExtensions
    );

    ~Application();


    void RunMainLoop();


protected:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    void cleanup();

    void createInstance( const std::string& appName, const VectorCString& validationLayers );

    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );

    void setupDebugMessenger();

    void createSurface();

    void pickPhysicalDevice( const VectorCString& deviceExtensions );

    void createLogicalDevice( const VectorCString& validationLayers, const VectorCString& deviceExtensions );

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline( const std::string& vertShaderPath, const std::string& fragShaderPath );

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void createSyncObjects();

    void drawFrame();

    VkShaderModule createShaderModule( const std::vector<char>& code );

    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );

    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );

    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );

    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );

    bool isDeviceSuitable( VkPhysicalDevice device, const VectorCString& deviceExtensions );

    bool checkDeviceExtensionSupport( VkPhysicalDevice device, const VectorCString& deviceExtensions );

    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport( const VectorCString& validationLayers );

    static std::vector<char> readFile( const std::string& filename );
};

#endif // APPLICATION_H