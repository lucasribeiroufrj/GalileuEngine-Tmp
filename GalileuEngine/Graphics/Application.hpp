//
//  Application.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/09/24.
//

#pragma once

// Project-wise includes.
#include "UtilMacros.hpp"

// GLFW includes.
#define VK_USE_PLATFORM_MACOS_MVK
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// STD library includes.
#include <vector>
#include <optional>
#include <concepts>
#include <functional>

namespace GE
{
namespace Vulkan
{

/** A concept to restrict a function template, assuring its accepts a  VkCommandBuffer input and has no returning value.. */
template<typename TFunctional>
concept CCallableWithVkCommandBuffer = requires(TFunctional functional, VkCommandBuffer commandBuffer)
{
    { functional(commandBuffer) } -> std::convertible_to<void>;
};

/**
 * This class wrappers a Vulkan application, having a windows capable of 3D/2D rendering.
 * It curently uses GLFW library in order support windowing system, as well as keyboard and mouse input.
 *
 * Some nice references:
 * - An understanding-vulkan-objects post by Adam Sawicki: https://gpuopen.com/learn/understanding-vulkan-objects/.
 * - Some excellent vulkan diagrams: https://github.com/David-DiGioia/vulkan-diagrams.
 * - Might be interesting as well: https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-preface.html.
 */
class FApplication
{
public:     // Exposed API.
    /** The function to be called in order to show the rendering window. */
    void run();
    
private:     // Internal helper types.
    /**
     * A helper struct to store whether the physical device supports graphics operations and surface presentation or not.
     * The process of running graphics operations and then presenting the result is here called rendering.
     */
    struct FQueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamily;     // The index of a Graphics queue, if the Physical Device supports it.
        std::optional<uint32_t> PresentFamily;      // The index of a Surface queue, if the Physical Device supports it.
        
        /** Returns true if the physical device supports graphics operations and surface presentation. */
        bool isRenderingSupported() const
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };
    
    /** Used to query swap chain support details. */
    struct FSwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR SurfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> SurfaceFormats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

private:    // Internal API: methods used by prepareFrame()'s implementation.
    
    /** Wait for a frame to finish; */
    void waitForFrameToFinish();
    
    /**
     * Acquire an available presentable image from the swap chain, and retrieve the index of that image.
     * The ImageAvailableSemaphore semaphore will be signaled as soon as the image is acquired.
     */
    std::optional<uint32_t> acquireNextSwapChainImage();
    
    /** Reset the prepareFrame()'s fence in order to starting work on the frame again. */
    void resetInFlightFence();
    
    /** Encapsulates a vkResetCommandBuffer call. */
    void resetCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferResetFlags commandBufferResetFlags = 0) const;
    
    /** Writes the commands to be executed into a command buffer. */
    void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
    void queueCommandBufferSubmit();
    void queuePresentation(const uint32_t imageIndex);
    
private:    // Internal API.
    void initWindow();
    void initVulkan();
    void prepareFrame();
    void mainLoop();
    void createInstance();
    
    bool isEveryRequiredValidationLayerSupported();
    
    /** Returns a list of physical device extensions required to present rendering results to a surface. */
    std::vector<const char*> createRequiredDeviceExtensions();
    
    /** Checks whether or not the device has all the physical device extensions required to present rendering results to a surface. */
    bool isEveryRequiredDeviceExtensionSupported(const VkPhysicalDevice& device);
    
    void VulkanCleanup();
    void glfwCleanup();
    void cleanup();
    
    /** Query the physical sevice surface present capabilities, modes, formats. */
    FSwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device);
    
    /** Get the color depth. */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    
    /** Get the condition for "swapping" images to the screen. */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    
    /** Get the resolution of images in swap chain. */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    /** Initializes the class instance member: VkSwapchainKHR SwapChain. */
    void createSawpChain();
    
    void cleanupSwapChain();
    void recreateSwapChain();
    
    /** A helper function to create an image view.  */
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    
    /** Initializes the class instance member: std::vector<VkImageView> SwapChainImageViews. */
    void createImageViews();
    
    /** Create the class instance member: VkRenderPass RenderPass. */
    void createRenderPass();
    
    /** Initializes the class instance member VkDescriptorSetLayout DescriptorSetLayout. */
    void createDescriptorSetLayout();
    
    /**
     * Initializes the class instance members:
     *  - VkPipelineLayout PipelineLayout;
     *  - VkPipeline Pipeline.
     */
    void createGraphicsPipeline();
    
    /** Initializes the class instance member: std::vector<VkFramebuffer> SwapChainFrameBuffers. */
    void createFrameBuffers();
    
    /** Initializes the class instance member: VkCommandPool CommandPool. */
    void createCommandPool();
    
    /** A helper function used to find a suitable image format. */
    VkFormat findSupportedImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
    /** A helper function used to find a suitable depth image format. */
    VkFormat findDepthFormat();
    
    /** Initializes the depth resources. */
    void createDepthResources();
    
    /** Encapsulates vkGetPhysicalDeviceMemoryProperties(). */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    void createImage(uint32_t width, const uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    
    /** A helper function useful for changing image layout.  */
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    
    /** Create the texture image example. */
    void createTextureImage();
    
    /** Create the texture image view example. */
    void createTextureImageView();
    
    /** Create the texture sampler. */
    void createTextureImageSampler();
    
    /** Initializes the class instance member: std::vetor<VkCommandBuffer> CommandBuffers. */
    void createCommandBuffers();
    
    /**
     * A helper function used to queue GPU commands in order to run them immediately.
     * The function will "queue-wait-idle on the graphics queue until all the commands have finished."
     */
    void queueSingleTimeCommands(CCallableWithVkCommandBuffer auto&& callback);
    
    /** A helper function used to copy data between two VkBuffers. */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    /** A helper function used to copy data from a VkBuffers to a VkImage. */
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    /** A helper function used to create and initialize VkBuffers. */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    /** A helper function used to create and initialize VkBuffers using existing data from a container, e.g. std::vector, std::array and etc. */
    template<typename TContainer>
    void createGenericBuffer(const TContainer& container, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
    /**
     * Initializes the class instance members:
     * - VkBuffer VertexBuffer;
     * - VkDeviceMemory VertexBufferMemory.
     */
    void createVertexBuffer();
    
    /**
     * Initializes the class instance members:
     * - VkBuffer IndexBuffer;
     * - VkDeviceMemory IndexBufferMemory.
     */
    void createIndexBuffer();
    
    /** Initializes the (default) uniform buffers. */
    void createUniformBuffers();
    
    /** Initializes the (default) descriptor pool (for the uniform buffers). */
    void createDescriptorPool();
    
    /** Initializes the (default) descriptor sets (for the uniform buffers). */
    void createDescriptorSets();
    
    /** Updates uniform buffers before drawing the new frame. */
    void updateUniformBuffers(const uint32_t currentFrame);
    
    /**
     * Initializes the class instance members:
     * - VkSemaphore ImageAvailableSemaphore;
     * - VkSemaphore RenderFinishedSemaphore;
     * - VkFence InFlightFence.
     */
    void createSyncObjects();
    
    /** Create shader modules. */
    VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
    
    VkDebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo();
    std::vector<const char*> createRequiredApiExtensionNames();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    FQueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
    std::vector<VkDeviceQueueCreateInfo> createDeviceQueueCreateInfos(const FQueueFamilyIndices& indices);
    void createLogicalDevice();
    bool isDeviceSuitable(const VkPhysicalDevice& device);
    
public:     // Static debug-related functions.
    static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    
private:    // Static validation-layer data.
    static std::vector<const char*> createValidationLayerNames();

private:    // Static general helper functions.
    static bool hasStencilComponent(VkFormat format);
    
private:    // Static const data.
    static constexpr int DefaultWindowWidth = 800;
    static constexpr int DefaultWindowHeight = 600;
    
    static constexpr int DefaultMaxFramesInFlight = 2;
    
    const uint64_t InfiniteTimeout = UINT64_MAX;
    
    static constexpr VkFormat DefaultTextureImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    
    static constexpr VkImageTiling DefaultDepthTiling = VK_IMAGE_TILING_OPTIMAL;
    
private:    // Private data
    bool IsValidationLayerOn = TRUE_IF_GE_BUILD_DEBUG;
    GLFWwindow* Window;
    VkSurfaceKHR Surface;
    
    VkInstance Instance;
    
    VkDebugUtilsMessengerEXT DebugMessenger;
    
    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
    
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    
    bool IsFrameBufferResized = false;
    VkSwapchainKHR SwapChain;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImageViews;
    std::vector<VkFramebuffer> SwapChainFrameBuffers;
    
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> GraphicsCommandBuffers;
    
    VkImage DepthImage;
    VkDeviceMemory DepthImageMemory;
    VkImageView DepthImageView;
    
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureImageSampler;
    
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;
    
    // For "shader-global" data, e.g. MVP transformationss (1 per frame).
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBufferMemories;
    std::vector<void*> MappedUniformBuffers;
    
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;
    
    VkRenderPass RenderPass;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkPipelineLayout PipelineLayout;
    VkPipeline Pipeline;
    
private:    // More private data, about synchronozation.
    /**
     * The synchronization data members declared below are used during this set of steps:
     *  1) Wait (for a fence) for the previous frame to finish;
     *  2) Retrieve (acquire) an image from the swap chain;
     *  3) Reset a command buffer;
     *  4) Prepare (record) the command buffer to render the scene onto this image;
     *  5) Send (submit) the prepared (recorded) command buffer for execution;
     *  6) Display (present) the swap chain image.
     */
    
    /** Signaled when a swap chain image has been acquired and is ready for rendering. */
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    
    /** Signaled when the rendering has finished and presentation can happen. */
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    
    /** The CPU waits for the GPU to finish processing comands for a previous frame before submitting commands for the next frame. */
    std::vector<VkFence> InFlightFences;
    
    /** Current frame being worked on by the CPU. */
    uint32_t currentFrame;
};  // End of class FApplication

}   // End of namespace Vulkan
}   // End of namespace GE
