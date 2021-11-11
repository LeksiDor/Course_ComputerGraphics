#include "ApplicationBase.h"

#include <glm/glm.hpp>
#include <iostream>


// Assignment 2.


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



class AppExample : public svk::ApplicationBase
{
public:

    virtual VkVertexInputBindingDescription getVertexBindingDescription() const override
    {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }

    virtual std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() const override
    {
        return { { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) } };
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
