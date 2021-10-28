#ifndef SVK_SCREENRENDERER_H
#define SVK_SCREENRENDERER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>


struct GLFWwindow;

namespace svk {


class ScreenRenderer
{
public:
    // Base class for application-unique items to be updated before each frame.
    class RenderEntry
    {
    public:
        virtual ~RenderEntry() = 0;
        virtual std::shared_ptr<RenderEntry> Clone() = 0;
        virtual void UpdateFrame() = 0;
    };


public:

    ScreenRenderer();


private:

    struct SwapChainEntry
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        //VkBuffer uniformBuffer = VK_NULL_HANDLE;
        //VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
        //VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence imageInFlight = VK_NULL_HANDLE;
    };

    struct RenderFrameEntry
    {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;
    };


private:

    GLFWwindow* window = nullptr;

    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<SwapChainEntry> swapChainEntries;
    std::vector<RenderFrameEntry> renderFrameEntries;

    VkRenderPass renderPass;
    //VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    //CommandPool commandPool;

    size_t currentFrame = 0;

};

} // namespace svk


#endif // SVK_SCREENRENDERER_H