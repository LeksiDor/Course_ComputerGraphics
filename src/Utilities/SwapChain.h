#ifndef SVK_SWAPCHAIN_H
#define SVK_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string>


struct GLFWwindow;

namespace svk {


class CommandPool;
class Image;


struct SwapChainEntry
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence imageInFlight = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};


struct FenceEntry
{
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};


struct SwapChainInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
    VkSurfaceFormatKHR surfaceFormat;
    VkFormat imageFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    uint32_t numEntries;

    void Update();
};


class RenderEntryManager
{
public:
    virtual std::vector<VkVertexInputBindingDescription> getVertexBindingDescriptions() const = 0;
    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const = 0;
    virtual std::vector<VkDescriptorType> getDescriptorTypes() const = 0;
    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const = 0;
    virtual void InitRenderEntries( const SwapChainInfo& swapChainInfo ) = 0;
    virtual void ClearRenderEntries() = 0;
    virtual void UpdateRenderEntry( const SwapChainInfo& swapChainInfo, const SwapChainEntry& swapChainEntry, const int swapEntryIndex ) = 0;
    virtual void BindSwapEntryCommandBuffer( const SwapChainEntry& entry ) = 0;
};


class SwapChain
{
public:

    SwapChain() = default;

    void Init(
        std::shared_ptr<CommandPool> commandPool,
        RenderEntryManager* renderEntryManager,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath
    );

    ~SwapChain();

    void DrawFrame();

    void SetResizeFlag()
    {
        framebufferResized = true;
    }


    VkPipelineLayout PipelineLayout() const { return pipelineLayout; }


private:
    void cleanupSwapChain();
    void recreateSwapChain();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createDepthResources();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    VkFormat findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );
    VkFormat findDepthFormat();

private:

    GLFWwindow* window = nullptr;

    std::string vertShaderPath;
    std::string fragShaderPath;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    SwapChainInfo swapChainInfo = {};

    std::vector<SwapChainEntry> swapChainEntries;
    std::vector<FenceEntry> fenceEntries;
    RenderEntryManager* renderEntryManager = nullptr;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    std::shared_ptr<CommandPool> commandPool;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    std::shared_ptr<Image> depthImage;

    size_t currentFrame = 0;

    bool framebufferResized = false;
};


} // namespace svk

#endif // SVK_SWAPCHAIN_H