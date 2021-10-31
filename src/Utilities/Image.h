#ifndef SVK_IMAGE_H
#define SVK_IMAGE_H

#include <vulkan/vulkan.h>
#include <string>
#include <memory>


namespace svk {


class CommandPool;


class Image
{
public:

    Image( const Image& ) = delete;

    Image();

    Image(
        const uint32_t width,
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usage,
        const VkImageLayout layout,
        const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        const VkMemoryPropertyFlags memoryUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        const VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
    );

    ~Image();


    static std::shared_ptr<Image> CreateFromFile( const CommandPool& commandPool, const std::string& filepath );

    void Reset(
        const uint32_t width,
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags imageUsage,
        const VkImageLayout layout,
        const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        const VkMemoryPropertyFlags memoryUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        const VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
    );

    void Clear();


    uint32_t Width()  const { return width; }
    uint32_t Height() const { return height; }

    const VkImage& Handle() const { return image; }
    const VkDeviceMemory& DeviceMemory() const { return deviceMemory; }
    const VkDescriptorImageInfo& Info() const { return info; }

    static VkImage CreateImageHandle( const uint32_t width, const uint32_t height, const VkFormat format, const VkImageUsageFlags imageUsage, const VkImageTiling tiling );
    static VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT );
    static VkSampler CreateSampler();
    static VkDeviceMemory CreateBindedDeviceMemory( VkImage image, const VkMemoryPropertyFlags memoryUsage );


    void TransitionLayout( const CommandPool& commandPool, VkImageLayout newLayout );

private:
    uint32_t width = 0;
    uint32_t height = 0;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    VkDescriptorImageInfo info = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };
};


} // namespace svk

#endif // SVK_IMAGE_H