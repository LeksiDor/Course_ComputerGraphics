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

const float recSizeX = 0.8f;
const float recSizeY = 0.6f;

const std::vector<Vertex> vertices_local = {

    { { -recSizeX*0.5, -recSizeY*0.5, 0.0f }, { 0.0f, 0.0f } },
    { {  recSizeX*0.5, -recSizeY*0.5, 0.0f }, { 1.0f, 0.0f } },
    { { -recSizeX*0.5,  recSizeY*0.5, 0.0f }, { 0.0f, 1.0f } },

    { {  recSizeX*0.5,  recSizeY*0.5, 0.0f }, { 1.0f, 1.0f } },
    { {  recSizeX*0.5, -recSizeY*0.5, 0.0f }, { 1.0f, 0.0f } },
    { { -recSizeX*0.5,  recSizeY*0.5, 0.0f }, { 0.0f, 1.0f } },

};

std::vector<Vertex> vertices = vertices_local;

const std::vector<uint32_t> indices = { 1, 0, 2, 3, 4, 5, };


const std::vector<glm::vec3> corners = {
    { -recSizeX*0.5, -recSizeY*0.5, 0.0f },
    {  recSizeX*0.5, -recSizeY*0.5, 0.0f },
    { -recSizeX*0.5,  recSizeY*0.5, 0.0f },
    {  recSizeX*0.5,  recSizeY*0.5, 0.0f },
};


const float _2Pi = float( 2.0 * M_PI );

const float MaxLinearSpeed = 0.5f;
// In radians per second.
const float MaxRotationSpeed = 0.8f;

const float TrianglesPullToCenter = 2.0f; // Constant speed that pulls to center.
const float TrianglesExplodeSpeedMin = 1.0f; // Min out-of-center starting speed.
const float TrianglesExplodeSpeedMax = 5.0f; // Max out-of-center starting speed.
const float TrianglesExplodeSpeedDeterioration = 2.0f; // How much speed deteriorates per second.

std::vector<glm::vec3> TrianglesExplodeSpeed; // Out-of-center speed.
std::vector<glm::vec3> TrianglesExplodeShift; // Relative position to center.
int numTriangles = 2;


const std::string TEXTURE_PATH = std::string(ROOT_DIRECTORY) + "/media/texture.jpg";



// Gives uniformly distributed random value from [0,1] interval.
float RandomValue()
{
    static std::random_device rd;  // Will be used to obtain a seed for the random number engine
    static std::mt19937 gen( rd() ); // Standard mersenne_twister_engine seeded with rd()
    static std::uniform_real_distribution<> dis( 0.0, 1.0 );
    return dis( gen );
}

void RegularizeAngularValue( float& angle )
{
    angle -= int( angle / _2Pi ) * _2Pi;
}


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

    virtual void InitAppResources() override
    {
        linSpeed.x = (-1.0f + 2.0f*RandomValue()) * MaxLinearSpeed;
        linSpeed.y = (-1.0f + 2.0f*RandomValue()) * MaxLinearSpeed;
        rotSpeed = (-1.0f + 2.0f*RandomValue()) * MaxRotationSpeed;

        TrianglesExplodeSpeed.resize( numTriangles );
        TrianglesExplodeShift.resize( numTriangles );
        for ( int i = 0; i < numTriangles; ++i )
        {
            TrianglesExplodeSpeed[i] = glm::vec3( 0.0f, 0.0f, 0.0f );
            TrianglesExplodeShift[i] = glm::vec3( 0.0f, 0.0f, 0.0f );
        }

        texture = svk::Image::CreateFromFile( *commandPool, TEXTURE_PATH );

        glfwSetMouseButtonCallback( window, mouse_button_callback );
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

        // Adjust linear speed a little bit.
        linSpeed.x = std::clamp( linSpeed.x + deltaTime*MaxLinearSpeed*(-1.0f + 2.0f*RandomValue()), -MaxLinearSpeed, MaxLinearSpeed );
        linSpeed.y = std::clamp( linSpeed.y + deltaTime*MaxLinearSpeed*(-1.0f + 2.0f*RandomValue()), -MaxLinearSpeed, MaxLinearSpeed );
        // Adjust rotation speed a little bit.
        rotSpeed = std::clamp( rotSpeed + deltaTime*MaxRotationSpeed*(-1.0f + 2.0f*RandomValue()), -MaxRotationSpeed, MaxRotationSpeed );

        linPos += linSpeed * deltaTime;
        rotPos += deltaTime * rotSpeed;
        RegularizeAngularValue( rotPos );

        // Check corners for collision.
        for ( const auto& corner : corners )
        {
            glm::vec3 pos = corner;
            pos = glm::rotateZ( pos, rotPos );
            pos += linPos;

            bool collided = false;
            if ( pos.x >  1.0f ) { linSpeed.x = -std::abs( linSpeed.x ); collided = true; }
            if ( pos.x < -1.0f ) { linSpeed.x =  std::abs( linSpeed.x ); collided = true; }
            if ( pos.y >  1.0f ) { linSpeed.y = -std::abs( linSpeed.y ); collided = true; }
            if ( pos.y < -1.0f ) { linSpeed.y =  std::abs( linSpeed.y ); collided = true; }
        }

        // Move triangles.
        for ( int tri_ind = 0; tri_ind < numTriangles; ++tri_ind )
        {
            auto& explodeSpeed = TrianglesExplodeSpeed[tri_ind];
            auto& explodeShift = TrianglesExplodeShift[tri_ind];
            // Deteriorate explode speed.
            float speedNorm = glm::length( explodeSpeed );
            const glm::vec3 speedDir = (speedNorm > 0.01f) ? (explodeSpeed/speedNorm) : glm::vec3(0,0,0);
            const float deterioration = ( 1.0f + glm::length( explodeShift ) ) * TrianglesExplodeSpeedDeterioration;
            speedNorm = std::max( 0.0f, speedNorm - deltaTime*deterioration );
            explodeSpeed = speedNorm * speedDir;
            // Apply explode speed, then pull to center.
            explodeShift += explodeSpeed * deltaTime;
            float centerDist = glm::length( explodeShift );
            const glm::vec3 shiftDir = (centerDist > 0.0001f) ? (explodeShift / centerDist) : glm::vec3(0,0,0);
            centerDist = std::max( 0.0f, centerDist - deltaTime*TrianglesPullToCenter );
            explodeShift = centerDist * shiftDir;
            // Apply transform to each triangle vertex.
            for ( int i = 0; i < 3; ++i )
            {
                glm::vec3 pos = vertices_local[3*tri_ind+i].pos;
                pos = glm::rotateZ( pos, rotPos );
                pos += linPos;
                pos += explodeShift;
                vertices[3*tri_ind+i].pos = pos;
            }
        }

        swapchain->ReuploadVertexBuffer( vertices );
    }

    static void mouse_button_callback( GLFWwindow* window, int button, int action, int mods )
    {
        auto app = reinterpret_cast<AppExample*>( glfwGetWindowUserPointer( window ) );

        if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
        {
            double xpos, ypos;
            glfwGetCursorPos( window, &xpos, &ypos );

            int width, height;
            glfwGetFramebufferSize( window, &width, &height );

            glm::vec2 clickPos = glm::vec2(0.5+xpos,0.5+ypos) / glm::vec2(width,height);
            clickPos = -glm::vec2(1.0f,1.0f) + 2.0f*clickPos;

            std::cout << "Mouse click: " << clickPos.x << " " << clickPos.y << std::endl;

            // Explode triangles.
            for ( int tri_ind = 0; tri_ind < numTriangles; ++tri_ind )
            {
                const float dir_angle = RandomValue() * _2Pi;
                const glm::vec3 dir = { cos(dir_angle), sin(dir_angle), 0.0f };
                const float value = TrianglesExplodeSpeedMin + RandomValue() * ( TrianglesExplodeSpeedMax - TrianglesExplodeSpeedMin );
                TrianglesExplodeSpeed[tri_ind] += value*dir;
            }
        }
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
