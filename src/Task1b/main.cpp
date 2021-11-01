#include "ApplicationBase.h"

#include <glm/glm.hpp>
#include <iostream>
#include <chrono>
#include <random>


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};


const std::vector<Vertex> vertices_local = {
    { {  0.0f, -0.2f }, { 1.0f, 0.0f, 0.0f} },
    { {  0.2f,  0.2f }, { 0.0f, 1.0f, 0.0f} },
    { { -0.2f,  0.2f }, { 0.0f, 0.0f, 1.0f} },
};

std::vector<Vertex> vertices = vertices_local;

const std::vector<uint32_t> indices = { 0, 2, 1 };

const float _2Pi = float( 2.0 * M_PI );


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
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
        };
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
        std::random_device rd;  // Will be used to obtain a seed for the random number engine
        std::mt19937 gen( rd() ); // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dis( -1.0, 1.0 );

        rotPos = dis(gen) * M_PI;
        rotSpeed = 1.0f;

        linSpeed.x = dis(gen) * 0.2f;
        linSpeed.y = dis(gen) * 0.2f;
    }

    virtual void UpdateFrameData() override
    {
        static const auto startTime = std::chrono::high_resolution_clock::now();
        static auto prevTime = startTime;
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
        prevTime = currentTime;

        rotPos += deltaTime * rotSpeed;
        rotPos -= int( rotPos / _2Pi ) * _2Pi;
        const float cosX = cosf( rotPos );
        const float sinX = sinf( rotPos );

        for ( int i = 0; i < 3; ++i )
        {
            glm::vec2& pos = vertices[i].pos;
            const glm::vec2& pos_loc = vertices_local[i].pos;

            linPos.x += linSpeed.x * deltaTime;
            linPos.y += linSpeed.y * deltaTime;

            pos.x = linPos.x + cosX*pos_loc.x - sinX*pos_loc.y;
            pos.y = linPos.y + sinX*pos_loc.x + cosX*pos_loc.y;

            // Change speed direction on collision.
            bool collided = false;
            if ( pos.x >  1.0f ) { linSpeed.x = -std::abs( linSpeed.x ); collided = true; }
            if ( pos.x < -1.0f ) { linSpeed.x =  std::abs( linSpeed.x ); collided = true; }
            if ( pos.y >  1.0f ) { linSpeed.y = -std::abs( linSpeed.y ); collided = true; }
            if ( pos.y < -1.0f ) { linSpeed.y =  std::abs( linSpeed.y ); collided = true; }

            if ( collided )
                std::cout << "bounce: " << pos.x << " " << pos.y << std::endl;
        }

        swapchain->ReuploadVertexBuffer( vertices );
    }

    // Linear position + speed.
    glm::vec2 linPos = { 0.0f, 0.0f };
    glm::vec2 linSpeed = { 0.0f, 0.0f };
    // Rotational position + speed.
    float rotPos = 0.0f;
    float rotSpeed = 0.0f;
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
