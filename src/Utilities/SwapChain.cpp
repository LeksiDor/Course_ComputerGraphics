#include "SwapChain.h"
#include "VulkanContext.h"
#include "VulkanBase.h"


namespace svk {

void SwapChain::Info::Update()
{
    const auto window = theVulkanContext().Window();
    const auto surface = theVulkanContext().Surface();
    const auto physicalDevice = theVulkanContext().PhysicalDevice();

    const auto support = QuerySwapChainSupport( physicalDevice, surface );
    capabilities = support.capabilities;
    formats = support.formats;
    presentModes = support.presentModes;

    surfaceFormat = ChooseSwapSurfaceFormat( support.formats );
    imageFormat = surfaceFormat.format;
    presentMode = ChooseSwapPresentMode( support.presentModes );
    extent = ChooseSwapExtent( window, support.capabilities );

    numEntries = support.capabilities.minImageCount + 1;
    if ( support.capabilities.maxImageCount > 0 && numEntries > support.capabilities.maxImageCount )
        numEntries = support.capabilities.maxImageCount;
}


} // namespace svk
