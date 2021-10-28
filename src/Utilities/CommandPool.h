#ifndef SVK_COMMANDPOOL_H
#define SVK_COMMANDPOOL_H

#include <vulkan/vulkan.h>


namespace svk {


class CommandPool
{
public:

    CommandPool( const CommandPool& ) = delete;

    CommandPool() = default;

    CommandPool( const uint32_t familyIndex );

    ~CommandPool();

    void Reset( const uint32_t familyIndex );

    void Clear();


    const VkCommandPool& Handle() const { return commandPool; }


    VkCommandBuffer CreateCommandBuffer() const;

    void FreeCommandBuffer( VkCommandBuffer& commandBuffer ) const;


    static void BeginCommandBuffer( const VkCommandBuffer commandBuffer, const bool isSingleUse );
    static void EndCommandBuffer( const VkCommandBuffer commandBuffer );


private:
    VkCommandPool commandPool = VK_NULL_HANDLE;
};

} // namespace svk

#endif // SVK_COMMANDPOOL_H