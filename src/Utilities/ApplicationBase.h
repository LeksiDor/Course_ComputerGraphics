#ifndef SVK_APPLICATIONBASE_H
#define SVK_APPLICATIONBASE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "CommandPool.h"
#include "SwapChain.h"
#include "VulkanContext.h"


namespace svk {

class ApplicationBase : public RenderEntryManager
{
public:

    virtual void Init(
        const uint32_t width,
        const uint32_t height,
        const std::string& appName,
        const std::vector<const char*>& validationLayers,
        const std::vector<const char*>& deviceExtensions
    )
    {
        InitWindow( width, height, appName );
        InitVulkan( appName, validationLayers, deviceExtensions );
    }


    virtual void InitWindow(
        const uint32_t width,
        const uint32_t height,
        const std::string& appName )
    {
        glfwInit();
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        window = glfwCreateWindow( width, height, appName.c_str(), nullptr, nullptr );
        glfwSetWindowUserPointer( window, this );
        glfwSetFramebufferSizeCallback( window, ApplicationBaseResizeCallback );
    }


    virtual void InitVulkan(
        const std::string& appName,
        const std::vector<const char*>& validationLayers,
        const std::vector<const char*>& deviceExtensions
    )
    {
        auto& context = theVulkanContext();
        context.Init( appName, window, validationLayers,deviceExtensions );
        commandPool.reset( new CommandPool( context.GraphicsFamily().value() ) );
        InitAppResources();
        swapchain.reset( new SwapChain() );
        InitSwapChain();
    }


    virtual void InitAppResources()
    {
        return;
    }

    virtual void InitSwapChain() = 0;


    virtual void MainLoop()
    {
        const auto device = theVulkanContext().LogicalDevice();
        while ( !glfwWindowShouldClose( window ) )
        {
            glfwPollEvents();
            UpdateFrameData();
            swapchain->DrawFrame();
        }
        vkDeviceWaitIdle( device );
    }


    virtual void UpdateFrameData()
    {
        return;
    }


    virtual void Destroy()
    {
        swapchain.reset();
        DestroyAppData();
        commandPool.reset();
        theVulkanContext().Destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
    }


    virtual void DestroyAppData()
    {
        return;
    }


    static void ApplicationBaseResizeCallback( GLFWwindow* window, int width, int height )
    {
        auto app = reinterpret_cast<ApplicationBase*>( glfwGetWindowUserPointer( window ) );
        app->swapchain->SetResizeFlag();
    }

protected:
    GLFWwindow* window = nullptr;
    std::shared_ptr<SwapChain> swapchain;
    std::shared_ptr<CommandPool> commandPool;


};


} // namespace svk

#endif // SVK_APPLICATIONBASE_H