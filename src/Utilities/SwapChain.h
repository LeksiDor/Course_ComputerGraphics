#ifndef SVK_SWAPCHAIN_H
#define SVK_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>


namespace svk {


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


class RenderEntryManager;


class SwapChain
{
public:

    struct Info
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

};


class RenderEntryManager
{
public:
    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const = 0;
    virtual void InitRenderEntries( const SwapChain::Info& swapChainInfo ) = 0;
    virtual void ClearRenderEntries() = 0;
    virtual void UpdateRenderEntry( const SwapChain::Info& swapChainInfo, const SwapChainEntry& swapChainEntry, const int swapEntryIndex ) = 0;
};


} // namespace svk

#endif // SVK_SWAPCHAIN_H