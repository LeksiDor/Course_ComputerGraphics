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
    virtual VkVertexInputBindingDescription getVertexBindingDescription() const = 0;
    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const = 0;

    virtual std::vector<VkDescriptorType> getDescriptorTypes() const
    {
        return {};
    }

    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const
    {
        return {};
    }

    virtual void InitRenderEntries( const SwapChainInfo& swapChainInfo )
    {
        return;
    }

    virtual void ClearRenderEntries()
    {
        return;
    }

    virtual void UpdateRenderEntry( const SwapChainInfo& swapChainInfo, const SwapChainEntry& swapChainEntry, const int swapEntryIndex )
    {
        return;
    }
};


class SwapChain
{
public:

    SwapChain() = default;

    template< typename Vertex >
    void Init(
        std::shared_ptr<CommandPool> commandPool,
        RenderEntryManager* renderEntryManager,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath )
    {
        const VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
        Init_Internal( commandPool, renderEntryManager, indices, vertexBufferSize, vertices.data(), vertShaderPath, fragShaderPath );
    }

    ~SwapChain();

    void DrawFrame();

    void SetResizeFlag()
    {
        framebufferResized = true;
    }

    template< typename Vertex >
    void ResetVertexIndexBuffers(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices )
    {
        const VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
        resetVertexIndexBuffer( indices, bufferSize, vertices.data() );
    }

public:
    void Init_Internal(
        std::shared_ptr<CommandPool> commandPool,
        RenderEntryManager* renderEntryManager,
        const std::vector<uint32_t>& indices,
        const VkDeviceSize vertexBufferSize,
        const void* vertexBufferData,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath
    );

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

    void resetVertexIndexBuffer(
        const std::vector<uint32_t>& indices,
        const VkDeviceSize vertexBufferSize,
        const void* vertexBufferData
    );

    void cleanupVertexIndexBuffers();

public:

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

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    uint32_t numDrawIndices = 0;

    std::shared_ptr<CommandPool> commandPool;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    std::shared_ptr<Image> depthImage;

    size_t currentFrame = 0;

    bool framebufferResized = false;
};


} // namespace svk

#endif // SVK_SWAPCHAIN_H