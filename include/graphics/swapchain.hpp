#pragma once

#include <graphics/image.hpp>
#include <graphics/device.hpp>

#include <vulkan/vulkan.h>
#include <vector>

namespace niqqa
{
namespace graphics
{
struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept;

class Swapchain
{
public:
    bool create(Device &device,
                VkSurfaceKHR surface, 
                VkExtent2D actual_extent,
                VkFormat user_defined_format,
                VkPresentModeKHR user_defined_present_mode,
                VkRenderPass render_pass) noexcept;
    
    void cleanup() noexcept;

    VkSwapchainKHR swapchain() const noexcept;
    VkExtent2D extent() const noexcept;
    VkFormat present_format() const noexcept;
    VkFormat depth_format() const noexcept;

    VkFramebuffer framebuffers(uint32_t index) const noexcept;

private:
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    VkDevice m_device{VK_NULL_HANDLE};
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    VkPresentModeKHR m_present_mode;
    VkFormat m_present_format{VK_FORMAT_UNDEFINED};
    VkFormat m_depth_format{VK_FORMAT_UNDEFINED};
    VkExtent2D m_extent;
    VkImage m_depth_image{VK_NULL_HANDLE};
    VkImageView m_depth_image_view{VK_NULL_HANDLE};
    VkDeviceMemory m_depth_memory{VK_NULL_HANDLE};

    std::vector<Image> m_present_images;
    std::vector<VkFramebuffer> m_framebuffers;

    VkFormat find_supported_format(VkPhysicalDevice gpu, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags feature_flag) noexcept;
    VkFormat find_depth_format(VkPhysicalDevice gpu) noexcept;

    VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats, VkFormat desired_format) noexcept;
    VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_modes, VkPresentModeKHR desired_mode) noexcept;
    VkExtent2D choose_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D actual_extent) noexcept;

    bool create_image_views() noexcept;
    bool create_framebuffers(VkRenderPass render_pass) noexcept;
    bool create_image(VkPhysicalDevice gpu,
                      uint32_t width,
                      uint32_t height,
                      VkFormat format,
                      VkImageTiling tiling,
                      VkImageUsageFlags usage_flags,
                      VkMemoryPropertyFlags property_flags,
                      VkImage &image,
                      VkDeviceMemory &image_memory) noexcept;
    bool create_depth_resources(VkPhysicalDevice gpu) noexcept;
};
} // namespace graphics
} // namespace niqqa