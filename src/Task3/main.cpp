#include "ApplicationBase.h"
#include "Image.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <chrono>
#include <random>


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    uint32_t isTexture;
};


const std::vector<Vertex> vertices_local = {
    // Triangle.
    { {  0.0f, -0.2f, 0.0f }, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f }, 0 },
    { {  0.2f,  0.2f, 0.0f }, { 0.0f, 1.0f, 0.0f}, { 0.0f, 0.0f }, 0 },
    { { -0.2f,  0.2f, 0.0f }, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f }, 0 },
    // Rectangle.
    { { -0.4f, -0.3f, 0.0f }, { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f }, 1 },
    { {  0.4f, -0.3f, 0.0f }, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f }, 1 },
    { { -0.4f,  0.3f, 0.0f }, { 0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f }, 1 },
    { {  0.4f,  0.3f, 0.0f }, { 0.6f, 0.6f, 0.6f}, { 1.0f, 1.0f }, 1 },
};

std::vector<Vertex> vertices = vertices_local;

const std::vector<uint32_t> indices = {
    // Triangle.
    0, 2, 1,
    // Rectangle.
    4, 3, 5,   4, 5, 6,
    3, 4, 5,   5, 4, 6,
};


const float _2Pi = float( 2.0 * M_PI );

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
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) },
            { 3, 0, VK_FORMAT_R32_UINT, offsetof(Vertex, isTexture) },
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
        tri_rotPos = RandomValue() * M_PI;
        tri_rotSpeed = 1.0f;

        tri_linSpeed.x = RandomValue() * 0.2f;
        tri_linSpeed.y = RandomValue() * 0.2f;

        rec_rotSpeed.x = RandomValue() * MaxRotationSpeed;
        rec_rotSpeed.y = RandomValue() * MaxRotationSpeed;
        rec_rotSpeed.z = RandomValue() * MaxRotationSpeed;

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

        tri_rotPos += deltaTime * tri_rotSpeed;
        RegularizeAngularValue( tri_rotPos );

        rec_rotPos += deltaTime * rec_rotSpeed;
        RegularizeAngularValue( rec_rotPos.x );
        RegularizeAngularValue( rec_rotPos.y );
        RegularizeAngularValue( rec_rotPos.z );

        // Triangle.
        for ( int i = 0; i < 3; ++i )
        {
            glm::vec3& pos = vertices[i].pos;
            const glm::vec3& pos_loc = vertices_local[i].pos;
            const glm::vec2 pos2_loc = { pos_loc.x, pos_loc.y };

            tri_linPos.x += tri_linSpeed.x * deltaTime;
            tri_linPos.y += tri_linSpeed.y * deltaTime;

            glm::vec2 pos2 = tri_linPos + glm::rotate( pos2_loc, tri_rotPos );

            // Change speed direction on collision.
            bool collided = false;
            if ( pos2.x >  1.0f ) { tri_linSpeed.x = -std::abs( tri_linSpeed.x ); collided = true; }
            if ( pos2.x < -1.0f ) { tri_linSpeed.x =  std::abs( tri_linSpeed.x ); collided = true; }
            if ( pos2.y >  1.0f ) { tri_linSpeed.y = -std::abs( tri_linSpeed.y ); collided = true; }
            if ( pos2.y < -1.0f ) { tri_linSpeed.y =  std::abs( tri_linSpeed.y ); collided = true; }

            pos = { pos2.x, pos2.y, 0.0f };
        }

        // Rectangle.
        for ( int i = 0; i < 4; ++i )
        {
            glm::vec3& pos = vertices[3+i].pos;
            const glm::vec3& pos_loc = vertices_local[3+i].pos;
            pos = pos_loc;
            pos = glm::rotateX( pos, rec_rotPos.x );
            pos = glm::rotateY( pos, rec_rotPos.y );
            pos = glm::rotateZ( pos, rec_rotPos.z );
        }
        // Adjust rectangle rotation speed a little bit.
        rec_rotSpeed.x = std::clamp( rec_rotSpeed.x + deltaTime*MaxRotationSpeed*RandomValue(), -MaxRotationSpeed, MaxRotationSpeed );
        rec_rotSpeed.y = std::clamp( rec_rotSpeed.y + deltaTime*MaxRotationSpeed*RandomValue(), -MaxRotationSpeed, MaxRotationSpeed );
        rec_rotSpeed.z = std::clamp( rec_rotSpeed.z + deltaTime*MaxRotationSpeed*RandomValue(), -MaxRotationSpeed, MaxRotationSpeed );

        swapchain->ReuploadVertexBuffer( vertices );
    }

    // Linear position + speed.
    glm::vec2 tri_linPos = { 0.0f, 0.0f };
    glm::vec2 tri_linSpeed = { 0.0f, 0.0f };
    // Rotational position + speed.
    float tri_rotPos = 0.0f;
    float tri_rotSpeed = 0.0f;

    // Rectangle.
    glm::vec3 rec_rotPos = {};
    glm::vec3 rec_rotSpeed = {};

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
