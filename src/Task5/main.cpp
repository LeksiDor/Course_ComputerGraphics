#include "ApplicationBase.h"
#include "Image.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <chrono>
#include <random>


struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
};

const std::vector<Vertex> vertices_local = {
    { { -0.4f, -0.3f, 0.0f }, { 0.0f, 0.0f } },
    { {  0.4f, -0.3f, 0.0f }, { 1.0f, 0.0f } },
    { { -0.4f,  0.3f, 0.0f }, { 0.0f, 1.0f } },
    { {  0.4f,  0.3f, 0.0f }, { 1.0f, 1.0f } },
};

std::vector<Vertex> vertices = vertices_local;

const std::vector<uint32_t> indices = { 1, 0, 2,   1, 2, 3, };


const float _2Pi = float( 2.0 * M_PI );

const float MaxLinearSpeed = 0.3f;
// In radians per second.
const float MaxRotationSpeed = 0.5f;

const std::string TEXTURE_PATH = std::string(ROOT_DIRECTORY) + "/media/texture.jpg";


class AppExample : public svk::ApplicationBase
{
public:

    virtual VkVertexInputBindingDescription getVertexBindingDescription() const override
    {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }

    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const override
    {
        return {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) },
        };
    }

    virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorBindings() const override
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        return { samplerLayoutBinding };
    }

    virtual std::vector<VkWriteDescriptorSet> getDescriptorWrites( const VkDescriptorSet& descriptorSet, const int swapEntryIndex ) const override
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites(1);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &texture->Info();

        return descriptorWrites;
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

    // Gives uniformly distributed random value from [-1,1] interval.
    static float RandomValue()
    {
        static std::random_device rd;  // Will be used to obtain a seed for the random number engine
        static std::mt19937 gen( rd() ); // Standard mersenne_twister_engine seeded with rd()
        static std::uniform_real_distribution<> dis( -1.0, 1.0 );
        return dis( gen );
    }

    static void RegularizeAngularValue( float& angle )
    {
        angle -= int( angle / _2Pi ) * _2Pi;
    }

    virtual void InitAppResources() override
    {
        linSpeed.x = RandomValue() * MaxLinearSpeed;
        linSpeed.y = RandomValue() * MaxLinearSpeed;
        rotSpeed = RandomValue() * MaxRotationSpeed;

        texture = svk::Image::CreateFromFile( *commandPool, TEXTURE_PATH );
    }

    virtual void DestroyAppResources() override
    {
        texture.reset();
    }

    virtual void UpdateFrameData() override
    {
        static const auto startTime = std::chrono::high_resolution_clock::now();
        static auto prevTime = startTime;
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
        prevTime = currentTime;

        rotPos += deltaTime * rotSpeed;
        RegularizeAngularValue( rotPos );

        for ( int i = 0; i < 4; ++i )
        {
            glm::vec3& pos = vertices[i].pos;
            const glm::vec3& pos_loc = vertices_local[i].pos;

            linPos += linSpeed * deltaTime;

            pos = pos_loc;
            pos = glm::rotateZ( pos, rotPos );
            pos += linPos;

            // Change speed direction on collision.
            bool collided = false;
            if ( pos.x >  1.0f ) { linSpeed.x = -std::abs( linSpeed.x ); collided = true; }
            if ( pos.x < -1.0f ) { linSpeed.x =  std::abs( linSpeed.x ); collided = true; }
            if ( pos.y >  1.0f ) { linSpeed.y = -std::abs( linSpeed.y ); collided = true; }
            if ( pos.y < -1.0f ) { linSpeed.y =  std::abs( linSpeed.y ); collided = true; }
        }

        // Adjust linear speed a little bit.
        linSpeed.x = std::clamp( linSpeed.x + deltaTime*MaxLinearSpeed*RandomValue(), -MaxLinearSpeed, MaxLinearSpeed );
        linSpeed.y = std::clamp( linSpeed.y + deltaTime*MaxLinearSpeed*RandomValue(), -MaxLinearSpeed, MaxLinearSpeed );
        // Adjust rotation speed a little bit.
        rotSpeed = std::clamp( rotSpeed + deltaTime*MaxRotationSpeed*RandomValue(), -MaxRotationSpeed, MaxRotationSpeed );

        swapchain->ReuploadVertexBuffer( vertices );
    }

    // Linear position + speed.
    glm::vec3 linPos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 linSpeed = { 0.0f, 0.0f, 0.0f };
    // Rotational position + speed.
    float rotPos = 0.0f;
    float rotSpeed = 0.0f;

    // Texture.
    std::shared_ptr<svk::Image> texture;
};


int main()
{
    AppExample app;
    try
    {
        app.Init(
            800, // width
            800, // height
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
