#ifndef SVK_SWAPCHAIN_H
#define SVK_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>


namespace svk {

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


} // namespace svk

#endif // SVK_SWAPCHAIN_H