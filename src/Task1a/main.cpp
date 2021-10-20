#include "Application.h"

#include <iostream>


const Application::VectorCString validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const Application::VectorCString deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};



int main()
{
    Application app(
        800, // width
        600, // height
        PROJECT_NAME, // App name.
        std::string(PROJECT_NAME) + "/shader.vert.spv", // vert shader path
        std::string(PROJECT_NAME) + "/shader.frag.spv", // frag shader path
        validationLayers,
        deviceExtensions
        );

    try
    {
        app.RunMainLoop();
    }
    catch ( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}