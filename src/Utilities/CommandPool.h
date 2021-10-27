#ifndef SVK_COMMANDPOOL_H
#define SVK_COMMANDPOOL_H

#include <vulkan/vulkan.h>


namespace svk {

class CommandBuffer;
class CommandPool;


class CommandBuffer
{
private:
    friend class CommandPool;
    CommandBuffer( VkCommandPool commandPool );

public:
    ~CommandBuffer();

};


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


private:
    VkCommandPool commandPool = VK_NULL_HANDLE;
};

} // namespace svk

#endif // SVK_COMMANDPOOL_H