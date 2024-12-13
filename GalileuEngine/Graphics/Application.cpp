//
//  Application.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/09/24.
//

#include "Application.hpp"

// Shaderc includes:
#include <shaderc/shaderc.hpp>

// GLFW includes:
#ifdef MAC_OS
    #define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

// GLM includes:
#define GLM_FORCE_DEPTH_ZERO_TO_ONE     // Needed to use Vulkan's depth range of 0.0 to 1.0 instead of OpenGL's depth range of -1.0 to 1.0.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// STD includes:
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb/stb_image.h"

// STD library includes:
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <set>
#include <unordered_set>
#include <cstring>
#include <optional>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <chrono>

namespace GE
{
namespace Vulkan
{

/**
 * A helper structure used to store Movel-View-Projection uniform data.
 */
struct FUniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * A helper structure used when creating a vertex buffer.
 */
struct FVertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    
    static std::array<VkVertexInputBindingDescription, 1> getBindingDescriptions()
    {
        std::array<VkVertexInputBindingDescription, 1> bindingDescriptions =
        {
            VkVertexInputBindingDescription
            {
                .binding = 0,
                .stride = sizeof(FVertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
        };
        
        return bindingDescriptions;
    }
    
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions =
        {
            VkVertexInputAttributeDescription
            {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(FVertex, pos),
            },
            VkVertexInputAttributeDescription
            {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(FVertex, color),
            },
            VkVertexInputAttributeDescription
            {
                .binding = 0,
                .location = 2,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(FVertex, texCoord),
            }
        };
        
        return attributeDescriptions;
    }
};

struct FShaderInfo
{
    std::string shaderName;
    shaderc_shader_kind kind;
    VkShaderStageFlagBits stage;
};

const std::vector<FVertex> vertices =
{
    // First rectangle:
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    
    // Second rectangle:
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices =
{
    // First rectangle:
    0, 1, 2,    // 1st triangle
    2, 3, 0,    // 2nd triangle
    
    // Second rectangle:
    4, 5, 6,    // 1st triangle
    6, 7, 4     // 2nd triangle
};

/** Helper function to load the content of a file . */
static std::string readFile(const std::string& filepath)
{
    // Start reading (ate flag) at the end of the file in order to easily get the size of the file, which is done next.
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filepath);
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::string buffer(fileSize, '\0');
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static void saveSpirvToFile(const std::string& filepath, const std::vector<uint32_t>& spirvCode)
{
    using value_type = std::remove_cvref_t<decltype(spirvCode)> ::value_type;
    std::ofstream spirv_file(filepath, std::ios::binary);
    spirv_file.write(reinterpret_cast<const char*>(spirvCode.data()), spirvCode.size() * sizeof(value_type));
    spirv_file.close();
}

static std::vector<uint32_t> compileShader(const std::string& sourceCode, const FShaderInfo& shaderInfo, shaderc_optimization_level optimizationLevel = shaderc_optimization_level_performance)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    options.SetOptimizationLevel(optimizationLevel);
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(sourceCode, shaderInfo.kind, shaderInfo.shaderName.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("Failed to compile shader! Error: " + result.GetErrorMessage());
    }
    
    return {result.cbegin(), result.cend()};
}

// Global constexpr constants:
constexpr uint32_t MainGraphicsQueueIndex = static_cast<uint32_t>(0);
constexpr uint32_t MainPresentQueueIndex = static_cast<uint32_t>(0);

// Global constexpr constants for the swap chain setup:
constexpr VkFormat PreferredSurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
constexpr VkColorSpaceKHR PreferredSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
constexpr VkPresentModeKHR PreferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr uint32_t ImageArrayLayers = static_cast<uint32_t>(1);     // Always 1 unless a stereoscopic 3D application is being developed.
constexpr VkImageUsageFlags DefaultImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

// Global const declarations for defining default shader folder and names:
const std::string DefaultSourceShaderFolder = "shaders/src/";
const std::string DefaultSourceVertexShaderName = "DefaultVertexShader.vert";
const std::string DefaultSourceFragmentShaderName = "DefaultFragmentShader.frag";
const std::string DefaultShaderFolder = "shaders/bin/";
const std::string DefaultVertexShaderName = "vertexShader.spv";
const std::string DefaultFragmentShaderName = "fragmentShader.spv";

// Global const declarations for defining default texture folder and names:
const std::string DefaultTextureFolder = "textures/";
const std::string DefaultTextureName = "statue-512x512.jpg";

void FApplication::run()
{
    try
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Crash -- Vulkan application has launched an exception\n - " << exception.what() << std::endl;
    }
}

void FApplication::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    Window = glfwCreateWindow(DefaultWindowWidth, DefaultWindowHeight, "Galileu Engine", nullptr, nullptr);
    
    glfwSetWindowUserPointer(Window, this);
    glfwSetFramebufferSizeCallback
    (
        Window,
        [](GLFWwindow* window, int, int)
        {
            auto app = reinterpret_cast<FApplication*>(glfwGetWindowUserPointer(window));
            app->IsFrameBufferResized = true;
        }
    );
}

void FApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSawpChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFrameBuffers();
    createTextureImage();
    createTextureImageView();
    createTextureImageSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void FApplication::queueCommandBufferSubmit()
{
    const std::array<VkSemaphore, 1> renderFinishedSemaphores = {RenderFinishedSemaphores[currentFrame]};
    const std::array<VkSemaphore, 1> imageAvailableSemaphores = {ImageAvailableSemaphores[currentFrame]};
    const std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const std::array<VkCommandBuffer, 1> commandBuffers = {GraphicsCommandBuffers[currentFrame]};
    const std::array<VkSubmitInfo, 1> submitInfos =
    {
        VkSubmitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = imageAvailableSemaphores.size(),
            .pWaitSemaphores = imageAvailableSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = commandBuffers.size(),
            .pCommandBuffers = commandBuffers.data(),
            .signalSemaphoreCount = renderFinishedSemaphores.size(),
            .pSignalSemaphores = renderFinishedSemaphores.data(),
        }
    };
    const VkResult result = vkQueueSubmit(GraphicsQueue, submitInfos.size(), submitInfos.data(), InFlightFences[currentFrame]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to queue the submit! Error: " + std::to_string(result));
    }
}

void FApplication::queuePresentation(const uint32_t imageIndex)
{
    const std::array<VkSemaphore, 1> renderFinishedSemaphores = {RenderFinishedSemaphores[currentFrame]};
    const std::array<VkSwapchainKHR, 1> swapchains = {SwapChain};
    const VkPresentInfoKHR presentInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = renderFinishedSemaphores.size(),
        .pWaitSemaphores = renderFinishedSemaphores.data(),
        .swapchainCount = swapchains.size(),
        .pSwapchains = swapchains.data(),
        .pImageIndices = &imageIndex,
    };
    const VkResult result = vkQueuePresentKHR(PresentQueue, &presentInfo);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || (IsFrameBufferResized))
    {
        IsFrameBufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to queue the presentation! Error: " + std::to_string(result));
    }
}

void FApplication::waitForFrameToFinish()
{
    const std::array<VkFence, 1> inFlightFences = {InFlightFences[currentFrame]};
    const VkBool32 shouldWaitAll = VK_TRUE;
    
    const VkResult result = vkWaitForFences(LogicalDevice, inFlightFences.size(), inFlightFences.data(), shouldWaitAll, InfiniteTimeout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable wait for fences while waiting for frame to finish! Error: " + std::to_string(result));
    }
}

std::optional<uint32_t>  FApplication::acquireNextSwapChainImage()
{
    uint32_t retrievedImageIndex;                       // Value is retrieved using vkAcquireNextImageKHR.
    const VkFence fenceToBeSignaled = VK_NULL_HANDLE;   // Not necessary, only semaphore will be signaled.
    const VkResult result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, InfiniteTimeout, ImageAvailableSemaphores[currentFrame], fenceToBeSignaled, &retrievedImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return std::nullopt;
    }
    else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR))
    {
        throw std::runtime_error("Unable to acquire image from swap chain! Error: " + std::to_string(result));
    }
    
    return retrievedImageIndex;
}

void FApplication::resetInFlightFence()
{
    const std::array<VkFence, 1> inFlightFences = {InFlightFences[currentFrame]};
    
    const VkResult result = vkResetFences(LogicalDevice, inFlightFences.size(), inFlightFences.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to reset fences to start working on a new frame! Error: " + std::to_string(result));
    }
}

void FApplication::prepareFrame()
{
    waitForFrameToFinish();
    
    if (const std::optional<uint32_t> swapChainImageIndex = acquireNextSwapChainImage())
    {
        updateUniformBuffers(currentFrame);
        resetInFlightFence();

        const VkCommandBuffer graphicsCommandBuffer = GraphicsCommandBuffers[currentFrame];
        resetCommandBuffer(graphicsCommandBuffer);
        recordGraphicsCommandBuffer(graphicsCommandBuffer, *swapChainImageIndex);
        
        queueCommandBufferSubmit();
        queuePresentation(*swapChainImageIndex);
        
        currentFrame = (currentFrame + 1) % DefaultMaxFramesInFlight;
    }
}

void FApplication::mainLoop()
{
    while(!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
        prepareFrame();
    }
}

void FApplication::VulkanCleanup()
{
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    
    // Wait for all Vulkan operation to finish before any cleanup.
    vkDeviceWaitIdle(LogicalDevice);
    
    for (size_t i = 0; i < DefaultMaxFramesInFlight; ++i)
    {
        vkDestroySemaphore(LogicalDevice, ImageAvailableSemaphores[i], allocationCallbacks);
        vkDestroySemaphore(LogicalDevice, RenderFinishedSemaphores[i], allocationCallbacks);
        vkDestroyFence(LogicalDevice, InFlightFences[i], allocationCallbacks);
    }
    
    vkDestroyCommandPool(LogicalDevice, CommandPool, allocationCallbacks);
    
    cleanupSwapChain();
    
    vkDestroySampler(LogicalDevice, TextureImageSampler, allocationCallbacks);
    vkDestroyImageView(LogicalDevice, TextureImageView, allocationCallbacks);
    vkDestroyImage(LogicalDevice, TextureImage, allocationCallbacks);
    vkFreeMemory(LogicalDevice, TextureImageMemory, allocationCallbacks);
    
    for (size_t i = 0; i < DefaultMaxFramesInFlight; ++i)
    {
        vkDestroyBuffer(LogicalDevice, UniformBuffers[i], allocationCallbacks);
        vkFreeMemory(LogicalDevice, UniformBufferMemories[i], allocationCallbacks);
    }
    vkDestroyDescriptorPool(LogicalDevice, DescriptorPool, allocationCallbacks);
    vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, allocationCallbacks);
    
    vkDestroyBuffer(LogicalDevice, VertexBuffer, allocationCallbacks);
    vkFreeMemory(LogicalDevice, VertexBufferMemory, allocationCallbacks);
    
    vkDestroyBuffer(LogicalDevice, IndexBuffer, allocationCallbacks);
    vkFreeMemory(LogicalDevice, IndexBufferMemory, allocationCallbacks);
    
    vkDestroyPipeline(LogicalDevice, Pipeline, allocationCallbacks);
    vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, allocationCallbacks);
    
    vkDestroyRenderPass(LogicalDevice, RenderPass, allocationCallbacks);
    
    vkDestroyDevice(LogicalDevice, allocationCallbacks);
    
    if (IsValidationLayerOn)
    {
        DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, allocationCallbacks);
    }
    
    vkDestroySurfaceKHR(Instance, Surface, allocationCallbacks);
    vkDestroyInstance(Instance, allocationCallbacks);
    
    // Note: Missing queue cleanup? There is no need to cleanup any VkQueue since this is done automatically along with the logical device.
}

void FApplication::glfwCleanup()
{
    glfwDestroyWindow(Window);
    glfwTerminate();
}

void FApplication::cleanup()
{
    VulkanCleanup();
    glfwCleanup();
}

void FApplication::createInstance()
{
    if (IsValidationLayerOn && !isEveryRequiredValidationLayerSupported())
    {
        throw std::runtime_error("Validation layers requested, but not available");
    }
    
    const auto getRequiredVulkanApiVersion = []()
    {
        uint32_t apiVersion;
        const VkResult result = vkEnumerateInstanceVersion(&apiVersion);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Unable to get vulkan api version! Error: " + std::to_string(result));
        };
        
        const uint32_t vulkan_major_version = VK_VERSION_MAJOR(apiVersion);
        const uint32_t vulkan_minor_version = VK_VERSION_MINOR(apiVersion);
        const uint32_t vulkan_patch_version = VK_VERSION_PATCH(apiVersion);
        
        std::cerr << "Instance version: "
            << vulkan_major_version << "."
            << vulkan_minor_version << "."
            << vulkan_patch_version << "."
            << std::endl;
        
        return VK_MAKE_VERSION(vulkan_major_version, vulkan_minor_version, vulkan_patch_version);
    };
    
    const VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Main",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = getRequiredVulkanApiVersion(),
    };
    
    const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugUtilsMessengerCreateInfo();
    const std::vector<const char*> extensionNames = createRequiredApiExtensionNames();
    const std::vector<const char*> validationLayerNames = createValidationLayerNames();
    
    const VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        
        // BEG - Extensions setup
#ifdef MAC_OS
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
        // END - Extensions setup
        
        // BEG - Validation layers setup
        .enabledLayerCount = IsValidationLayerOn ? static_cast<uint32_t>(validationLayerNames.size()) : 0,
        .ppEnabledLayerNames = IsValidationLayerOn ? validationLayerNames.data() : nullptr,
        .pNext = IsValidationLayerOn ? static_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo) : 0,
        // END
    };
    
    // Finally create the (Vulkan) instance.
    const VkResult result = vkCreateInstance(&createInfo, nullptr, &Instance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance! Error: " + std::to_string(result));
    }
}

bool FApplication::isEveryRequiredValidationLayerSupported(){
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    const std::vector<const char*> validationLayerNames = createValidationLayerNames();
    
    for (const char* layerName : validationLayerNames)
    {
        bool layerFound = false;
        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound)
        {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> FApplication::createRequiredDeviceExtensions()
{
    std::vector<const char*> requiredDeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef MAC_OS
        "VK_KHR_portability_subset"
#endif
    };
    
    return requiredDeviceExtensions;
}

bool FApplication::isEveryRequiredDeviceExtensionSupported(const VkPhysicalDevice& physicalDevice)
{
    // 1) Get the number of supported extensions.
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    
    // 2) Get the supported extensions.
    std::vector<VkExtensionProperties> availableExtensions{extensionCount};
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    // 3) Convert to a set to remove duplicates, then try to "tick" every element in the set.
    const std::vector<const char*> requiredDeviceExtensions = createRequiredDeviceExtensions();
    std::unordered_set<std::string> requiredExtensionsSet{requiredDeviceExtensions.begin(), requiredDeviceExtensions.end()};
    for (const VkExtensionProperties& property : availableExtensions) {
        requiredExtensionsSet.erase(property.extensionName);
        if (requiredExtensionsSet.empty())
        {
            return true;
        }
    }
    
    // 4) If empty, the device has all the required extensions.
    // On the ontrary, if the set is not empty, some required extensions are missing.
    return requiredExtensionsSet.empty();
}

FApplication::FSwapChainSupportDetails FApplication::querySwapChainSupport(const VkPhysicalDevice& physicalDevice)
{
    FSwapChainSupportDetails details{};
    
    // Initializes details's first member:
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, Surface, &details.SurfaceCapabilities);
    
    // Initializes details's second Member:
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, nullptr);   // Get just the count first.
    if (formatCount > 0)
    {
        details.SurfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, details.SurfaceFormats.data());
    }
    
    // Initializes details's third member:
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentModeCount, nullptr);   // Get just the count first.
    if (presentModeCount > 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentModeCount, details.PresentModes.data());
    }
    
    return details;
}

VkSurfaceFormatKHR FApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // Try to find whether the following surface specification is available or not.
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
    {
        if ((availableFormat.format == PreferredSurfaceFormat) && (availableFormat.colorSpace == PreferredSurfaceColorSpace))
        {
            return availableFormat;
        }
    }
    
    // Could not find the preferred format, so Just return the first available.
    return availableFormats[0];
}

VkPresentModeKHR FApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // Try to find whether the following surface specification is available or not.
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == PreferredPresentMode)
        {
            return availablePresentMode;
        }
    }
    
    // Could not find the preferred mode, so just return the one guaranteed to be available.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D FApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    /**
     Vulkan requires us to match the window’s resolution by setting the width and height in the currentExtent member.
     However, some window managers allow flexibility, indicated by setting the currentExtent‘s width and height to the
     maximum uint32_t value. In this case, we select a resolution that fits the window, constrained by minImageExtent
     and maxImageExtent.
     */
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        // It’s important to ensure that the resolution is specified in the correct units.
        // This function handles this. This is specially important if it is dealing with a high DPI display, such as
        // Apple’s Retina display, where screen coordinates no longer directly correspond to pixels.
        int width, height;
        glfwGetFramebufferSize(Window, &width, &height);
        
        VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actualExtent;
    }
}

void FApplication::createSawpChain()
{
    FSwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(PhysicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.SurfaceFormats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.PresentModes);
    VkExtent2D extent2D = chooseSwapExtent(swapChainSupportDetails.SurfaceCapabilities);
    
    // As an optimization, it’s recommended to request at least one additional image to prevent Vulkan from waiting on
    // the driver to complete internal operations before acquiring another image for rendering.
    uint32_t imageCount = swapChainSupportDetails.SurfaceCapabilities.minImageCount + 1;
    const uint32_t maxImageCount = swapChainSupportDetails.SurfaceCapabilities.maxImageCount;
    
    // But, the maximum value must not be exceeded (with 0 being a special value that indicates no maximum).
    if (maxImageCount > 0 && imageCount > maxImageCount)
    {
        imageCount = maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = Surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent2D;
    swapchainCreateInfo.imageArrayLayers = ImageArrayLayers;
    swapchainCreateInfo.imageUsage = DefaultImageUsage;
    
    using FQueueFamilyIndices = FApplication::FQueueFamilyIndices;
    const FQueueFamilyIndices queueFamilyIndices = findQueueFamilies(PhysicalDevice);
    const uint32_t rawQueueFamilyIndices[] =
    {
        queueFamilyIndices.GraphicsFamily.value(),
        queueFamilyIndices.PresentFamily.value()
    };
    if (queueFamilyIndices.GraphicsFamily == queueFamilyIndices.PresentFamily)
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = rawQueueFamilyIndices;
    }
    
    swapchainCreateInfo.preTransform = swapChainSupportDetails.SurfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateSwapchainKHR(LogicalDevice, &swapchainCreateInfo, allocationCallbacks, &SwapChain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain! Error: " + std::to_string(result));
    }
    
    // The implementation can create more images than the minimum was specified above.
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &imageCount, nullptr);
    SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &imageCount, SwapChainImages.data());
    
    SwapChainImageFormat = surfaceFormat.format;
    SwapChainExtent = extent2D;
}

void FApplication::cleanupSwapChain()
{
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    
    vkDestroyImageView(LogicalDevice, DepthImageView, allocationCallbacks);
    vkDestroyImage(LogicalDevice, DepthImage, allocationCallbacks);
    vkFreeMemory(LogicalDevice, DepthImageMemory, allocationCallbacks);
    
    for (auto framebuffer : SwapChainFrameBuffers)
    {
        vkDestroyFramebuffer(LogicalDevice, framebuffer, allocationCallbacks);
    }
    
    for (VkImageView& imageView : SwapChainImageViews)
    {
        vkDestroyImageView(LogicalDevice, imageView, allocationCallbacks);
    }
    
    vkDestroySwapchainKHR(LogicalDevice, SwapChain, allocationCallbacks);
}

void FApplication::recreateSwapChain()
{
    // TOOD: move to a function. This handles minimization.
    int width = 0, height = 0;
    glfwGetFramebufferSize(Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(Window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(LogicalDevice);
    cleanupSwapChain();
    createSawpChain();
    createImageViews();
    createDepthResources();
    createFrameBuffers();
}

VkImageView FApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    const VkImageViewCreateInfo imageViewCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspectFlags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    VkImageView outImageView;
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateImageView(LogicalDevice, &imageViewCreateInfo, allocationCallbacks, &outImageView);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a swap chain image view! Error: " + std::to_string(result));
    }
    
    return outImageView;
}

void FApplication::createImageViews()
{
    SwapChainImageViews.resize(SwapChainImages.size());
    
    for (size_t index = 0; index < SwapChainImages.size(); ++index)
    {
        SwapChainImageViews[index] = createImageView(SwapChainImages[index], SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void FApplication::createRenderPass()
{
    const std::array<VkAttachmentDescription, 2> attachmentDescriptions =
    {
        // Color attachment
        VkAttachmentDescription
        {
            .format = SwapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        
        // Depth attachment
        VkAttachmentDescription
        {
            .format = findDepthFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };
    
    const std::array<VkAttachmentReference, 1> colorAttachmentReferences =
    {
        VkAttachmentReference
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };
    
    const VkAttachmentReference depthAttachmentRefrence =
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    const std::array<VkSubpassDescription, 1> subpassDescriptions =
    {
        VkSubpassDescription
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = colorAttachmentReferences.size(),
            .pColorAttachments = colorAttachmentReferences.data(),
            .pDepthStencilAttachment = &depthAttachmentRefrence
        }
    };
    
    const std::array<VkSubpassDependency, 1> subpassDependencies =
    {
        VkSubpassDependency
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            
        }
    };
    
    const VkRenderPassCreateInfo renderPassCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachmentDescriptions.size(),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = subpassDescriptions.size(),
        .pSubpasses = subpassDescriptions.data(),
        .dependencyCount = subpassDependencies.size(),
        .pDependencies = subpassDependencies.data(),
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateRenderPass(LogicalDevice, &renderPassCreateInfo, allocationCallbacks, &RenderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a render pass! Error: " + std::to_string(result));
    }
}

void FApplication::createDescriptorSetLayout()
{
    const std::array<VkDescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings =
    {
        VkDescriptorSetLayoutBinding
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        VkDescriptorSetLayoutBinding
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImmutableSamplers = nullptr,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };
    
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = descriptorSetLayoutBindings.size(),
        .pBindings = descriptorSetLayoutBindings.data()
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateDescriptorSetLayout(LogicalDevice, &descriptorSetLayoutCreateInfo, allocationCallbacks, &DescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout! Error: " + std::to_string(result));
    }
}

void FApplication::createGraphicsPipeline()
{
    std::vector<FShaderInfo> shaders =
    {
        {DefaultSourceVertexShaderName, shaderc_vertex_shader, VK_SHADER_STAGE_VERTEX_BIT},
        {DefaultSourceFragmentShaderName, shaderc_fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const FShaderInfo& shaderInfo: shaders)
    {
        const std::string shaderCode = readFile(DefaultSourceShaderFolder + shaderInfo.shaderName);

        VkPipelineShaderStageCreateInfo shaderStage =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shaderInfo.stage,
            .module = createShaderModule(compileShader(shaderCode, shaderInfo)),
            .pName = "main",
        };
        
        shaderStages.push_back(std::move(shaderStage));
    }
    
    const std::array<VkVertexInputBindingDescription, 1> bindingDescriptions = FVertex::getBindingDescriptions();
    const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = FVertex::getAttributeDescriptions();
    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = bindingDescriptions.size(),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
    
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
       
    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    
    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };
    
    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    
    const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
    {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };
    
    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
    };
    
    const std::array<VkDynamicState, 2> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };
    
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &DescriptorSetLayout,
        .pushConstantRangeCount = 0,
    };
    
    // Used in many places ahead.
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    
    // Creates the PipelineLayout class member.
    {
        const VkResult result = vkCreatePipelineLayout(LogicalDevice, &pipelineLayoutCreateInfo, allocationCallbacks, &PipelineLayout);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the pipeline layout! Error: " + std::to_string(result));
        }
    }
    
    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {}
    };
    
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = PipelineLayout,
        .renderPass = RenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };
    
    // Creates the Pipeline class member.
    {
        const VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        const uint32_t createInfoCount = 1;
        const VkResult result = vkCreateGraphicsPipelines(LogicalDevice, pipelineCache, createInfoCount, &pipelineCreateInfo, allocationCallbacks, &Pipeline);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the pipeline! Error: " + std::to_string(result));
        }
    }
    
    for (const VkPipelineShaderStageCreateInfo& stage: shaderStages)
    {
        vkDestroyShaderModule(LogicalDevice, stage.module, allocationCallbacks);
    }
}

void FApplication::createFrameBuffers()
{
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    SwapChainFrameBuffers.resize(SwapChainImageViews.size());
    
    for (size_t i = 0; i < SwapChainImageViews.size(); ++i)
    {
        const std::array<VkImageView, 2> attachments = { SwapChainImageViews[i], DepthImageView };
        
        const VkFramebufferCreateInfo framebufferCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = RenderPass,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .width = SwapChainExtent.width,
            .height = SwapChainExtent.height,
            .layers = 1,
        };
        
        const VkResult result = vkCreateFramebuffer(LogicalDevice, &framebufferCreateInfo, allocationCallbacks, &SwapChainFrameBuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a framebuffer! Error: " + std::to_string(result));
        }
    }
}

void FApplication::createCommandPool()
{
    const FQueueFamilyIndices queueFamilyIndices = findQueueFamilies(PhysicalDevice);
    const VkCommandPoolCreateInfo commandPoolCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value(),
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateCommandPool(LogicalDevice, &commandPoolCreateInfo, allocationCallbacks, &CommandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a command pool! Error: " + std::to_string(result));
    }
}

VkFormat FApplication::findSupportedImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties outProperties;
        vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &outProperties);
        
        if ((tiling == VK_IMAGE_TILING_LINEAR) && ((outProperties.linearTilingFeatures & features) == features))
        {
            return format;
        }
        else if ((tiling == VK_IMAGE_TILING_OPTIMAL) && ((outProperties.optimalTilingFeatures & features) == features))
        {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find a suitable format!");
}

VkFormat FApplication::findDepthFormat()
{
    return findSupportedImageFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, DefaultDepthTiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void FApplication::createDepthResources()
{
    const VkFormat depthFormat = findDepthFormat();
    
    createImage(SwapChainExtent.width, SwapChainExtent.height, depthFormat, DefaultDepthTiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, DepthImage, DepthImageMemory);
    
    DepthImageView = createImageView(DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    
    // Being cared of in the render pass by means of subpass dependency.
    //transitionImageLayout(DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

uint32_t FApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find a suitable memory type!");
}

void FApplication::createImage(uint32_t width, const uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    // Create an image:
    {
        const VkImageCreateInfo imageCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = format,
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .samples = VK_SAMPLE_COUNT_1_BIT
        };
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkCreateImage(LogicalDevice, &imageCreateInfo, allocationCallbacks, &image);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an image! Error: " + std::to_string(result));
        }
    }
    
    // Allocate memory for the image just created:
    {
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(LogicalDevice, image, &memoryRequirements);
        
        const VkMemoryAllocateInfo memoryAllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)
        };
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkAllocateMemory(LogicalDevice, &memoryAllocateInfo, allocationCallbacks, &imageMemory);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate texture image memory! Error: " + std::to_string(result));
        }
    }
    
    // Bind the allocated memory to the image object:
    {
        const VkDeviceSize offset = 0;
        const VkResult result = vkBindImageMemory(LogicalDevice, image, imageMemory, offset);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind texture image memory! Error: " + std::to_string(result));
        }
    }
}

void FApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    queueSingleTimeCommands([=](VkCommandBuffer commandBuffer)
    {
        // They will be initialized below.
        VkAccessFlags srcAccessMask;
        VkAccessFlags dstAccessMask;
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        
        if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        {
            // This is a from-unknown-to-Write-to transition.
            srcAccessMask = 0;
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        {
            // This is a from-write-to-read-from transition.
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
        {
            srcAccessMask = 0;
            dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("Unsupported layout transition!");
        }
        
        VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        
        // Override aspect flag if handling a depth image.
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format))
            {
                aspectFlag |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        
        const std::array<VkImageMemoryBarrier, 1> imageMemoryBarriers =
        {
            VkImageMemoryBarrier
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout = oldLayout,
                .newLayout = newLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = image,
                .subresourceRange =
                {
                    .aspectMask = aspectFlag,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .srcAccessMask = srcAccessMask,
                .dstAccessMask = dstAccessMask
            }
        };

        const VkDependencyFlags dependencyFlags = 0;
        const std::array<VkMemoryBarrier, 0> memoryBarriers = {};
        const std::array<VkBufferMemoryBarrier, 0> bufferMemoryBarriers = {};
        vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarriers.size(), memoryBarriers.data(), bufferMemoryBarriers.size(), bufferMemoryBarriers.data(), imageMemoryBarriers.size(), imageMemoryBarriers.data()
        );
    });
}

void FApplication::createTextureImage()
{
    // Load the texture file:
    int outTextureWidth;
    int outTextureHeight;
    int outTextureChannels;
    const std::string pathname = DefaultTextureFolder + DefaultTextureName;
    stbi_uc* pixels = stbi_load(pathname.c_str(), &outTextureWidth, &outTextureHeight, &outTextureChannels, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        throw std::runtime_error("Failed to texture file!");
    }
    const int textureWidth = outTextureWidth;
    const int textureHeight = outTextureHeight;
    constexpr int numBytesPerPixels = 4;        // Number depends on STBI_rgb_alpha: R + G + A + Alpha.
    const VkDeviceSize imageSize = textureWidth * textureHeight * numBytesPerPixels;
    
    // Create a stating buffer (and allocate its memory):
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Copy the texture pixels to the stating buffer:
    void* data = nullptr;
    const VkDeviceSize offset = 0;
    const VkMemoryMapFlags flags = 0;
    vkMapMemory(LogicalDevice, stagingBufferMemory, offset, imageSize, flags, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(LogicalDevice, stagingBufferMemory);
    
    // Clean-up:
    stbi_image_free(pixels);
    
    // Define some constant(s):
    const VkImageLayout commonLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    
    // Create an image, i.e. initialize TextureImage and TextureImageMemory:
    createImage(textureWidth, textureHeight, DefaultTextureImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, TextureImage, TextureImageMemory);
    
    transitionImageLayout(TextureImage, DefaultTextureImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, commonLayout);
    
    copyBufferToImage(stagingBuffer, TextureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    
    transitionImageLayout(TextureImage, DefaultTextureImageFormat, commonLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Clean-up:
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    vkDestroyBuffer(LogicalDevice, stagingBuffer, allocationCallbacks);
    vkFreeMemory(LogicalDevice, stagingBufferMemory, allocationCallbacks);
}

void FApplication::createTextureImageView()
{
    TextureImageView = createImageView(TextureImage, DefaultTextureImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void FApplication::createTextureImageSampler()
{
    VkPhysicalDeviceProperties outProperties{};
    vkGetPhysicalDeviceProperties(PhysicalDevice, &outProperties);
    
    const VkSamplerCreateInfo samplerCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = outProperties.limits.maxSamplerAnisotropy,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateSampler(LogicalDevice, &samplerCreateInfo, allocationCallbacks, &TextureImageSampler);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a texture image sampler! Error: " + std::to_string(result));
    }
}

void FApplication::queueSingleTimeCommands(CCallableWithVkCommandBuffer auto&& callback)
{
    constexpr size_t commandBufferCount = 1;
    const VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = CommandPool,
        .commandBufferCount = commandBufferCount
    };
    
    std::array<VkCommandBuffer, commandBufferCount> commandBuffers;
    vkAllocateCommandBuffers(LogicalDevice, &allocInfo, commandBuffers.data());
    
    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffers[0], &beginInfo);
    
    callback(commandBuffers[0]);
    
    vkEndCommandBuffer(commandBuffers[0]);
    
    const std::array<VkSubmitInfo, commandBufferCount> submitInfos =
    {
        VkSubmitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = commandBuffers.size(),
            .pCommandBuffers = commandBuffers.data()
        }
    };
    vkQueueSubmit(GraphicsQueue, submitInfos.size(), submitInfos.data(), VK_NULL_HANDLE);
    vkQueueWaitIdle(GraphicsQueue);
    
    vkFreeCommandBuffers(LogicalDevice, CommandPool, commandBuffers.size(), commandBuffers.data());
}

void FApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    queueSingleTimeCommands([=](VkCommandBuffer commandBuffer)
    {
        constexpr size_t commandBufferCount = 1;
        const std::array<VkBufferCopy, commandBufferCount> copyRegions = { VkBufferCopy { .size = size } };
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, copyRegions.size(), copyRegions.data());
    });
}

void FApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    const VkBufferCreateInfo bufferCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    // Create buffer object.
    {
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkCreateBuffer(LogicalDevice, &bufferCreateInfo, allocationCallbacks, &buffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a vertex buffer! Error: " + std::to_string(result));
        }
    }
    
    // Allocate memory.
    {
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(LogicalDevice, buffer, &memoryRequirements);
        
        const VkMemoryAllocateInfo memoryAllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)
        };
        
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkAllocateMemory(LogicalDevice, &memoryAllocateInfo, allocationCallbacks, &bufferMemory);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate vertex buffer memory! Error: " + std::to_string(result));
        }
    }
    
    // Bind the allocated memory to the buffer object.
    {
        const VkDeviceSize offset = 0;
        const VkResult result = vkBindBufferMemory(LogicalDevice, buffer, bufferMemory, offset);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind buffer memory! Error: " + std::to_string(result));
        }
    }
}

void FApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    queueSingleTimeCommands([=](VkCommandBuffer commandBuffer)
    {
        const std::array<VkBufferImageCopy, 1> regions =
        {
            VkBufferImageCopy
            {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = {0, 0, 0},
                .imageExtent = {width, height, 1}
            }
        };
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
    });
}

template<typename TContainer>
void FApplication::createGenericBuffer(const TContainer& container, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    const VkDeviceSize bufferSize = sizeof(typename TContainer::value_type) * container.size();
    
    // Allocate a staging buffer.
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    // Fill the staging buffer (might be a RAM-to-VRAM memory copy when CPU and GPU do not share the same memory space).
    {
        void* data;
        const VkDeviceSize offset = 0;
        const VkMemoryMapFlags flags = 0;
        vkMapMemory(LogicalDevice, stagingBufferMemory, offset, bufferSize, flags, &data);
        memcpy(data, container.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(LogicalDevice, stagingBufferMemory);
    }
    
    // Allocate the buffer.
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
    
    // Move data from the stagingBuffer to the buffer.
    copyBuffer(stagingBuffer, buffer, bufferSize);
    
    // Clean up.
    vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
}

void FApplication::createVertexBuffer()
{
    createGenericBuffer(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VertexBuffer, VertexBufferMemory);
}

void FApplication::createIndexBuffer()
{
    // Note: It is recommended to store the vertices and indices in the same VkBuffer as an optimization strategy, the opposite of what it is being done here. See
    //     https://developer.nvidia.com/vulkan-memory-management
    // for more info.
    createGenericBuffer(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, IndexBuffer, IndexBufferMemory);
}

void FApplication::createUniformBuffers()
{
    const VkDeviceSize bufferSize = sizeof(FUniformBufferObject);
    const VkDeviceSize offset = 0;
    const VkMemoryMapFlags flags = 0;
    
    UniformBuffers.resize(DefaultMaxFramesInFlight);
    UniformBufferMemories.resize(DefaultMaxFramesInFlight);
    MappedUniformBuffers.resize(DefaultMaxFramesInFlight);
    
    for (size_t i = 0; i < DefaultMaxFramesInFlight; ++i)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UniformBuffers[i], UniformBufferMemories[i]);
        vkMapMemory(LogicalDevice, UniformBufferMemories[i], offset, bufferSize, flags, &MappedUniformBuffers[i]);
    }
}

void FApplication::createDescriptorPool()
{
    const uint32_t MaxFramesInFlight =  static_cast<uint32_t>(DefaultMaxFramesInFlight);
    
    const std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes =
    {
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = MaxFramesInFlight
        },
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MaxFramesInFlight
        }
    };
    
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = descriptorPoolSizes.size(),
        .pPoolSizes = descriptorPoolSizes.data(),
        .maxSets = MaxFramesInFlight
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateDescriptorPool(LogicalDevice, &descriptorPoolCreateInfo, allocationCallbacks, &DescriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool! Error: " + std::to_string(result));
    }
}

void FApplication::createDescriptorSets()
{
    const std::vector<VkDescriptorSetLayout> layouts(DefaultMaxFramesInFlight, DescriptorSetLayout);
    const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = DescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };
    
    DescriptorSets.resize(DefaultMaxFramesInFlight);
    const VkResult result = vkAllocateDescriptorSets(LogicalDevice, &descriptorSetAllocateInfo, DescriptorSets.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets! Error: " + std::to_string(result));
    }
    
    for (size_t i = 0; i < DefaultMaxFramesInFlight; ++i)
    {
        const VkDescriptorBufferInfo descriptorBufferInfo =
        {
            .buffer = UniformBuffers[i],
            .offset = 0,
            .range = sizeof(FUniformBufferObject)
        };
        
        const VkDescriptorImageInfo descriptorImageInfo =
        {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = TextureImageView,
            .sampler = TextureImageSampler
        };
        
        const std::array<VkWriteDescriptorSet, 2> writeDescriptorSets =
        {
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &descriptorBufferInfo
            },
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &descriptorImageInfo
            }
        };
        
        const std::array<VkCopyDescriptorSet, 0> copyDescriptorSets = {};
        vkUpdateDescriptorSets(LogicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), copyDescriptorSets.size(), copyDescriptorSets.data());
    }
}

void FApplication::updateUniformBuffers(const uint32_t currentFrame)
{
    // Model:
    using namespace std::chrono;
    static auto startTime = high_resolution_clock::now();
    const auto currentTime = high_resolution_clock::now();
    const float time = duration<float, seconds::period>(currentTime - startTime).count();
    const glm::mat4 model = {1.0f};                     // Identity matrix
    const float rotationSpeed = glm::radians(90.0f);        // Degrees / seconds.
    const glm::vec3 rotationAxis = {0.0f, 0.0f, 1.0f};
    
    // View:
    const glm::vec3 eye = {2.0f, 2.0f, 2.0f};           // Looking from.
    const glm::vec3 center = {0.0f, 0.0f, 0.0f};        // Looking at.
    const glm::vec3 up = {0.0f, 0.0f, 1.0f};
    
    // Projection:
    const float fieldOfView = glm::radians(45.0f);
    const float aspectRatio = SwapChainExtent.width / (float) SwapChainExtent.height;
    const float zNearPlane = 0.1f;
    const float zFarPlane = 10.0f;
    
    FUniformBufferObject ubo =
    {
        .model = glm::rotate(model, time * rotationSpeed, rotationAxis),
        .view = glm::lookAt(eye, center, up),
        .proj = glm::perspective(fieldOfView, aspectRatio, zNearPlane, zFarPlane)
    };
    ubo.proj[1][1] *= -1;       // glm has been built for OpenGL, but Vulkan's Y axis in inverted.
    memcpy(MappedUniformBuffers[currentFrame], &ubo, sizeof(ubo));
}

void FApplication::createCommandBuffers()
{
    GraphicsCommandBuffers.resize(DefaultMaxFramesInFlight);
    
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(GraphicsCommandBuffers.size()),
    };
    
    const VkResult result = vkAllocateCommandBuffers(LogicalDevice, &commandBufferAllocateInfo, GraphicsCommandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a command buffer! Error: " + std::to_string(result));
    }
}

void FApplication::createSyncObjects()
{
    ImageAvailableSemaphores.resize(DefaultMaxFramesInFlight);
    RenderFinishedSemaphores.resize(DefaultMaxFramesInFlight);
    InFlightFences.resize(DefaultMaxFramesInFlight);
    
    auto createSemaphore = [this](VkSemaphore* semaphore)
    {
        const VkSemaphoreCreateInfo semaphoreCreateInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkCreateSemaphore(LogicalDevice, &semaphoreCreateInfo, allocationCallbacks, semaphore);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a semaphore! Error: " + std::to_string(result));
        }
    };
    
    auto createFence = [this](VkFence* fence)
    {
        const VkFenceCreateInfo fenceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        const VkAllocationCallbacks* const allocationCallbacks = nullptr;
        const VkResult result = vkCreateFence(LogicalDevice, &fenceCreateInfo, allocationCallbacks, fence);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a fence! Error: " + std::to_string(result));
        }
    };
    
    // Create synchronization ojects.
    for (size_t i = 0; i < DefaultMaxFramesInFlight; ++i)
    {
        createSemaphore(&ImageAvailableSemaphores[i]);
        createSemaphore(&RenderFinishedSemaphores[i]);
        createFence(&InFlightFences[i]);
    }
    
    currentFrame = 0;
}

void FApplication::resetCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferResetFlags commandBufferResetFlags) const
{
    const VkResult result = vkResetCommandBuffer(commandBuffer, commandBufferResetFlags);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to reset the command buffer! Error: " + std::to_string(result));
    }
}

void FApplication::recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    // BEG - Definition of all lambdas.
    auto beginCommandBuffer = [commandBuffer]()
    {
        const VkCommandBufferBeginInfo commandBufferBeginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        const VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Unable to begin recording command buffer! Error: " + std::to_string(result));
        }
    };
    
    auto recordRenderSubPasses = [this, commandBuffer]()
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        
        const VkViewport viewport =
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(SwapChainExtent.width),
            .height = static_cast<float>(SwapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        const uint32_t firstViewport = 0;
        const uint32_t viewportCount = 1;
        vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, &viewport);
        
        const VkRect2D scissor =
        {
            .offset = {0, 0},
            .extent = SwapChainExtent,
        };
        const uint32_t firstScissor = 0;
        const uint32_t scissorCount = 1;
        vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, &scissor);
        
        const uint32_t firstBinding = 0;
        const std::array<VkBuffer, 1> vertexBuffers = { VertexBuffer };
        const VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, firstBinding, vertexBuffers.size(), vertexBuffers.data(), offsets);
        
        const VkDeviceSize indexOffset = 0;
        vkCmdBindIndexBuffer(commandBuffer, IndexBuffer, indexOffset, VK_INDEX_TYPE_UINT16);
        
        const uint32_t firstSet = 0;
        const std::array<VkDescriptorSet, 1> descriptorSets = { DescriptorSets[currentFrame] };
        const std::array<uint32_t, 0> dynamicOffsets = {};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, firstSet, descriptorSets.size(), descriptorSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
        
        const uint32_t indexCount = static_cast<uint32_t>(indices.size());
        const uint32_t instanceCount = 1;
        const uint32_t firstIndex = 0;
        const int32_t vertexOffset = 0;
        const uint32_t firstInstance = 0;
        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    };
    
    auto beginRenderPass = [this, commandBuffer, imageIndex]()
    {
        // NOTE: the order here, should be the same that of the attachments.
        const std::array<VkClearValue, 2> clearValues =
        {
            VkClearValue
            {
                .color = {0.0f, 0.0f, 0.0f, 1.0f}
            },
            
            VkClearValue
            {
                .depthStencil = {1.0f, 0}
            }
        };
        
        const VkRenderPassBeginInfo renderPassBeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = RenderPass,
            .framebuffer = SwapChainFrameBuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = SwapChainExtent,
            .clearValueCount = clearValues.size(),
            .pClearValues = clearValues.data(),
        };
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    };
    
    auto endRenderPass = [commandBuffer]()
    {
        vkCmdEndRenderPass(commandBuffer);
    };
    
    auto endCommandBuffer = [commandBuffer]()
    {
        const VkResult result = vkEndCommandBuffer(commandBuffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Unable to end recording command buffer! Error: " + std::to_string(result));
        }
    };
    // END - Definition of all lambdas.
    
    // Requests the commands recording.
    beginCommandBuffer();
    beginRenderPass();
    recordRenderSubPasses();
    endRenderPass();
    endCommandBuffer();
}

VkShaderModule FApplication::createShaderModule(const std::vector<uint32_t>& code)
{
    constexpr size_t byteStepSize = 4;
    static_assert( sizeof(code[0]) == byteStepSize );
    
    const VkShaderModuleCreateInfo shaderModuleCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size() * byteStepSize,    // It must be specified in bytes and multiple of 4.
        .pCode = code.data(),
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    VkShaderModule shaderModule{};
    const VkResult result = vkCreateShaderModule(LogicalDevice, &shaderModuleCreateInfo, allocationCallbacks, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module! Error: " + std::to_string(result));
    }
    
    return shaderModule;
}

VkDebugUtilsMessengerCreateInfoEXT FApplication::createDebugUtilsMessengerCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };
    
    return createInfo;
}

std::vector<const char*> FApplication::createRequiredApiExtensionNames()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (IsValidationLayerOn)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#ifdef MAC_OS
    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);        // Necessary from Vulkan 1.3.275.0 on.
#endif
    return extensions;
}

void FApplication::setupDebugMessenger()
{
    if (!IsValidationLayerOn)
    {
        return;
    }
    
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = createDebugUtilsMessengerCreateInfo();
    const VkResult result = createDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the debug messenger! Error: " + std::to_string(result));
    }
}

void FApplication::createSurface()
{
#if defined(MAC_OS)
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = glfwCreateWindowSurface(Instance, Window, allocationCallbacks, &Surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a window surface!");
    }
#else
    throw std::runtime_error("There no implementation for this platform yet!");
#endif
}

void FApplication::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
    if (deviceCount <= 0)
    {
        throw std::runtime_error("Unable to locate GPUs with Vulkan support!");
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());
    
    bool foundSuitableDevice = false;
    for (const VkPhysicalDevice& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            PhysicalDevice = device;
            foundSuitableDevice = true;
            break;
        }
    }
    
    if (!foundSuitableDevice)
    {
        throw std::runtime_error("Unable to find a suitable GPU!");
    }
}

FApplication::FQueueFamilyIndices FApplication::findQueueFamilies(const VkPhysicalDevice& physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    
    FQueueFamilyIndices queueFamilyIndices{};
    int i = 0;
    for (const VkQueueFamilyProperties& queueFamilyProperty : queueFamilyProperties)
    {
        // Check surface support and get the index to the related family.
        {
            VkBool32 isSurfaceSupportPresent = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, Surface, &isSurfaceSupportPresent);
            if (isSurfaceSupportPresent)
            {
                queueFamilyIndices.PresentFamily = i;
            }
        }
        
        if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices.GraphicsFamily = i;
        }
        
        if (queueFamilyIndices.isRenderingSupported())
        {
            break;
        }
        
        ++i;
    }
    
    return queueFamilyIndices;
}

std::vector<VkDeviceQueueCreateInfo> FApplication::createDeviceQueueCreateInfos(const FQueueFamilyIndices& indices)
{
    const std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};
    const std::array<float, 1> queuePrioriies = {1.0f};
    std::vector<VkDeviceQueueCreateInfo> infos;
    infos.reserve(uniqueQueueFamilies.size());
    for (const uint32_t queueFamily : uniqueQueueFamilies)
    {
        infos.push_back
        (
            VkDeviceQueueCreateInfo
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = queuePrioriies.size(),
                .pQueuePriorities = queuePrioriies.data(),
            }
         );
    }
    return infos;
}

void FApplication::createLogicalDevice()
{
    const FQueueFamilyIndices indices = findQueueFamilies(PhysicalDevice);
    const std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = createDeviceQueueCreateInfos(indices);
    const VkPhysicalDeviceFeatures physicalDeviceFeatures = { .samplerAnisotropy = VK_TRUE };
    const std::vector<const char*> requiredDeviceExtensions = createRequiredDeviceExtensions();
    const std::vector<const char*> validationLayerNames = createValidationLayerNames();
    const VkDeviceCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .pEnabledFeatures = &physicalDeviceFeatures,
        .enabledLayerCount = IsValidationLayerOn ? static_cast<uint32_t>(validationLayerNames.size()) : 0,
        .ppEnabledLayerNames = IsValidationLayerOn ? validationLayerNames.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
    };
    
    const VkAllocationCallbacks* const allocationCallbacks = nullptr;
    const VkResult result = vkCreateDevice(PhysicalDevice, &createInfo, allocationCallbacks, &LogicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a logical device!");
    }
    
    vkGetDeviceQueue(LogicalDevice, indices.GraphicsFamily.value(), MainGraphicsQueueIndex, &GraphicsQueue);
    vkGetDeviceQueue(LogicalDevice, indices.PresentFamily.value(), MainPresentQueueIndex, &PresentQueue);
}

bool FApplication::isDeviceSuitable(const VkPhysicalDevice& physicalDevice)
{
    /**
     * Properties and Features could be used here, e.g.
     *
     * VkPhysicalDeviceProperties properties;
     * VkPhysicalDeviceFeatures features;
     * vkGetPhysicalDeviceProperties(Logicaldevice, &properties);
     * vkGetPhysicalDeviceFeatures(Logicaldevice, &features);
     *
     * bool isDiscreteGPU = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
     * bool isGeometryShaderAvailable = features.geometryShader;
     */
    
    const FQueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    const bool isEveryExtensionSupported = isEveryRequiredDeviceExtensionSupported(physicalDevice);
    const bool isSwapChainAdequate = !isEveryExtensionSupported ? false : [this, &physicalDevice]()
    {
        FSwapChainSupportDetails details = querySwapChainSupport(physicalDevice);
        return !details.SurfaceFormats.empty() && !details.PresentModes.empty();
    }();
    
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
    const bool isSamplerAnisotropySupported = physicalDeviceFeatures.samplerAnisotropy;
    
    return indices.isRenderingSupported() && isEveryExtensionSupported && isSwapChainAdequate && isSamplerAnisotropySupported;
}

VkResult FApplication::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT != nullptr)
    {
        return vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void FApplication::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    const auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (vkDestroyDebugUtilsMessengerEXT != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
    }
    else
    {
        std::cerr << "validation layer: destroyDebugUtilsMessengerEXT is nullptr." << std::endl;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL FApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    
    return VK_FALSE;
}

std::vector<const char*> FApplication::createValidationLayerNames()
{
    return { "VK_LAYER_KHRONOS_validation" };
}

bool FApplication::hasStencilComponent(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

}   // End of namespace Vulkan
}   // End of namespace GE
