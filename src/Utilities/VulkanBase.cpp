#include "VulkanBase.h"


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