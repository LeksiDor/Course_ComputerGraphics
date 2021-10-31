#include "Image.h"

#include "VulkanBase.h"
#include "VulkanContext.h"
#include "CommandPool.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


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


std::shared_ptr<Image> Image::CreateFromFile( const CommandPool& commandPool, const std::string& filepath )
{
    const auto device = theVulkanContext().LogicalDevice();
    std::shared_ptr<Image> image( new Image() );

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load( filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error( "Failed to load texture image: " + filepath );
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    image->Reset( texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    image->TransitionLayout( commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
    copyBufferToImage( commandPool, stagingBuffer, image->Handle(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) );
    image->TransitionLayout( commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return image;
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


void Image::TransitionLayout( const CommandPool& commandPool, VkImageLayout newLayout )
{
    const VkImageLayout oldLayout = info.imageLayout;
    if ( oldLayout == newLayout )
        return;

    VkCommandBuffer commandBuffer = commandPool.CreateCommandBuffer();
    CommandPool::BeginCommandBuffer( commandBuffer, true );

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    CommandPool::EndCommandBuffer( commandBuffer );
    theVulkanContext().SubmitGraphicsQueue( commandBuffer );
    commandPool.FreeCommandBuffer( commandBuffer );

    info.imageLayout = newLayout;
}


} // namespace svk