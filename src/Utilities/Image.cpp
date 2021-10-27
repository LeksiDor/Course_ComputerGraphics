#include "Image.h"

#include "VulkanContext.h"

#include <stdexcept>


namespace svk {


Image::Image()
{
    Clear();
}


Image::Image( const uint32_t width, const uint32_t height, const VkFormat format, const VkImageUsageFlags usage, const VkImageLayout layout, const VkImageTiling tiling, const VkMemoryPropertyFlags memoryUsage, const VkImageAspectFlags aspectFlags )
{
    Reset( width, height, format, usage, layout, tiling, memoryUsage, aspectFlags );
}


Image::~Image()
{
    Clear();
}


void Image::Reset( const uint32_t width, const uint32_t height, const VkFormat format, const VkImageUsageFlags imageUsage, const VkImageLayout layout, const VkImageTiling tiling, const VkMemoryPropertyFlags memoryUsage, const VkImageAspectFlags aspectFlags )
{
    Clear();
    this->width = width;
    this->height = height;
    image = CreateImageHandle( width, height, format, imageUsage, tiling );
    deviceMemory = CreateBindedDeviceMemory( image, memoryUsage );
    info.imageLayout = layout;
    info.imageView = CreateImageView( image, format, aspectFlags );
    info.sampler = CreateSampler();
}


void Image::Clear()
{
    const auto device = theVulkanContext().LogicalDevice();

    if ( info.sampler != VK_NULL_HANDLE )
        vkDestroySampler( device, info.sampler, nullptr );
    if ( info.imageView != VK_NULL_HANDLE )
        vkDestroyImageView( device, info.imageView, nullptr );
    if ( image != VK_NULL_HANDLE )
        vkDestroyImage( device, image, nullptr );
    if ( deviceMemory != VK_NULL_HANDLE )
        vkFreeMemory( device, deviceMemory, nullptr );

    width = 0;
    height = 0;
    image = VK_NULL_HANDLE;
    deviceMemory = VK_NULL_HANDLE;
    info = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };
}


VkImage Image::CreateImageHandle( const uint32_t width, const uint32_t height, const VkFormat format, const VkImageUsageFlags imageUsage, const VkImageTiling tiling )
{
    const auto device = theVulkanContext().LogicalDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = imageUsage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage image = VK_NULL_HANDLE;
    if ( vkCreateImage( device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
        throw std::runtime_error( "Image: Failed to create image." );

    return image;
}


VkImageView Image::CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags )
{
    const auto device = theVulkanContext().LogicalDevice();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    if ( vkCreateImageView( device, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
        throw std::runtime_error( "Image: Failed to create image view." );

    return imageView;
}


VkSampler Image::CreateSampler()
{
    const auto device = theVulkanContext().LogicalDevice();
    const auto physicalDevice = theVulkanContext().PhysicalDevice();

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties( physicalDevice, &properties );

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler sampler = VK_NULL_HANDLE;
    if ( vkCreateSampler( device, &samplerInfo, nullptr, &sampler ) != VK_SUCCESS )
        throw std::runtime_error( "Image: Failed to create texture sampler." );

    return sampler;
}


VkDeviceMemory Image::CreateBindedDeviceMemory( VkImage image, const VkMemoryPropertyFlags memoryUsage )
{
    const auto device = theVulkanContext().LogicalDevice();

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( device, image, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = theVulkanContext().FindMemoryType( memRequirements.memoryTypeBits, memoryUsage );

    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    if ( vkAllocateMemory( device, &allocInfo, nullptr, &deviceMemory ) != VK_SUCCESS )
        throw std::runtime_error( "Image: Failed to allocate image memory." );

    if ( vkBindImageMemory( device, image, deviceMemory, 0 ) != VK_SUCCESS )
        throw std::runtime_error( "Image: Failed to bind image memory." );

    return deviceMemory;
}


} // namespace svk