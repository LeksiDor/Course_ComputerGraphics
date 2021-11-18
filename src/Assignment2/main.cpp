#include "ApplicationBase.h"
#include "VulkanBase.h"
#include "Image.h"

#include <glm/glm.hpp>
#include <chrono>
#include <iostream>

// Assignment 2.

const std::string TEXTURE_PATH = std::string( ROOT_DIRECTORY ) + "/media/emperor_of_mankind.jpg";


struct Vertex {
    glm::vec2 pos;
};


const std::vector<Vertex> vertices = {
    { { -1.0f, -1.0f } },
    { {  1.0f, -1.0f } },
    { { -1.0f,  1.0f } },
    { {  1.0f,  1.0f } },
};


const std::vector<uint32_t> indices = {
    1, 0, 2,
    1, 2, 3,
};


struct RenderEntry
{
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo bufferInfo = {};
};


struct UniformsStruct
{
    glm::mat4 lookAt; // LookAt matrix.
    glm::vec2 resolution; // Resolution of the screen.
    glm::vec2 mouse; // Mouse coordinates.
    float time; // Time since startup, in seconds.
    float gamma; // Gamma correction parameter.
    int shadow; // 0 = none, 1 = sharp, 2 = soft.
};


class AppExample : public svk::ApplicationBase
{
private:
    std::vector<RenderEntry> renderEntries;
    std::shared_ptr<svk::Image> texture;

public:

    virtual VkVertexInputBindingDescription getVertexBindingDescription() const override
    {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }

    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const override
    {
        return { { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) } };
    }

    virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorBindings() const override
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        return { uboLayoutBinding, samplerLayoutBinding };
    }

    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const override
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites(2);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &renderEntries[swapEntryIndex].bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &texture->Info();

        return descriptorWrites;
    }

    virtual void InitAppResources() override
    {
        texture = svk::Image::CreateFromFile( *commandPool, TEXTURE_PATH );
    }

    virtual void DestroyAppResources() override
    {
        texture.reset();
    }

    virtual void InitRenderEntries( const svk::SwapChainInfo& swapChainInfo ) override
    {
        ClearRenderEntries();
        renderEntries.resize( swapChainInfo.numEntries );
        const VkDeviceSize bufferSize = sizeof( UniformsStruct );
        for ( int i = 0; i < swapChainInfo.numEntries; ++i )
        {
            auto& entry = renderEntries[i];
            svk::createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, entry.uniformBuffer, entry.uniformBufferMemory );
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = entry.uniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = bufferSize;
            entry.bufferInfo = bufferInfo;
        }
    }

    virtual void ClearRenderEntries() override
    {
        const auto device = svk::theVulkanContext().LogicalDevice();
        for ( auto& entry : renderEntries )
        {
            vkDestroyBuffer( device, entry.uniformBuffer, nullptr );
            vkFreeMemory( device, entry.uniformBufferMemory, nullptr );
        }
        renderEntries.clear();
    }

    virtual void UpdateRenderEntry( const svk::SwapChainInfo& swapChainInfo, const svk::SwapChainEntry& swapChainEntry, const int swapEntryIndex ) override
    {
        const auto device = svk::theVulkanContext().LogicalDevice();
        auto& entry = renderEntries[swapEntryIndex];

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

        double xpos, ypos;
        glfwGetCursorPos( window, &xpos, &ypos );

        int width, height;
        glfwGetFramebufferSize( window, &width, &height );

        // Camera matrix.
        const bool isAnimateCamera = true;
        glm::vec3 up0 = glm::vec3( 0, 1, 0 );
        glm::vec3 eye = glm::vec3( 0, 0, -2 );
        glm::vec3 target = glm::vec3( 0, 0, 2 );
        if ( isAnimateCamera )
        {
            const float arg = 2.0f * time;
            eye.x = 2.0f * std::sin( arg );
            eye.y = std::cos( arg );
        }
        const glm::vec3 forward = glm::normalize( target - eye );
        const glm::vec3 right = glm::normalize( glm::cross( up0, forward ) );
        const glm::vec3 up = glm::normalize( glm::cross( forward, right ) );

        UniformsStruct uniforms{};
        uniforms.resolution = glm::vec2( width, height );
        uniforms.mouse = glm::vec2( xpos, ypos );
        uniforms.time = time;
        uniforms.gamma = 2.2f;
        uniforms.shadow = 2;
        uniforms.lookAt = glm::mat4(
            glm::vec4( right, 0.0f ),
            glm::vec4( up, 0.0f ),
            glm::vec4( forward, 0.0f ),
            glm::vec4( eye, 1.0f ) );

        void* data;
        vkMapMemory( device, entry.uniformBufferMemory, 0, sizeof( uniforms ), 0, &data );
        memcpy( data, &uniforms, sizeof( uniforms ) );
        vkUnmapMemory( device, entry.uniformBufferMemory );
    }

    virtual void InitSwapChain() override
    {
        swapchain->Init(
            commandPool,
            this,
            vertices,
            indices,
            std::string(PROJECT_NAME) + "/shader.vert.spv",
            std::string(PROJECT_NAME) + "/shader.frag.spv"
        );
    }
};


int main()
{
    AppExample app;
    try
    {
        app.Init(
            800, // width
            600, // height
            PROJECT_NAME, // App name.
            { "VK_LAYER_KHRONOS_validation" },
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME }
        );
        app.MainLoop();
        app.Destroy();
    }
    catch ( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
