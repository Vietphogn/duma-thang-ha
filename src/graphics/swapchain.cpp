#include <graphics/swapchain.hpp>

#include <graphics/device.hpp>
#include <log.hpp>

#include <cstdint>
#include <limits>
#include <algorithm>
#include <vector>
#include <format>
#include <string>

namespace niqqa
{
namespace graphics
{
SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    
    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

static uint32_t find_memory_type(VkPhysicalDevice gpu, uint32_t type_filter, VkMemoryPropertyFlags property_flags) noexcept
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

static VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) noexcept
{
    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = format;

    image_view_info.subresourceRange.aspectMask = aspect_flags;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    VkImageView image_view = VK_NULL_HANDLE;

    if (vkCreateImageView(device, &image_view_info, nullptr, &image_view) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return image_view;
}

bool Swapchain::create(Device &device, 
                       VkSurfaceKHR surface,
                       VkExtent2D actual_extent,
                       VkFormat user_defined_format,
                       VkPresentModeKHR user_defined_present_mode) noexcept
{
    m_device = device.device();
    m_surface = surface;

    SwapchainSupportDetails swap_chain_support = query_swapchain_support(device.gpu(), m_surface);
    QueueFamilyIndices indices = find_queue_families(device.gpu(), m_surface);
    uint32_t queue_family_indices[] = {
        indices.graphics_family.value(),
        indices.present_family.value()
    };

    VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swap_chain_support.formats, user_defined_format);
    VkPresentModeKHR present_mode = choose_swapchain_present_mode(swap_chain_support.present_modes, user_defined_present_mode);
    VkExtent2D extent = choose_swapchain_extent(swap_chain_support.capabilities, actual_extent);
    
    uint32_t image_count;
    image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = actual_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
    {
        LOG_ERROR("Swapchain", "Failed to create swapchain");
        return false;
    }

    std::vector<VkImage> images;

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, images.data());

    std::string msg = std::format("Swapchain created ({} image(s))", image_count);
    LOG_INFO("Swapchain", msg);

    m_present_images.resize(image_count);

    for (size_t i = 0; i < m_present_images.size(); ++i)
    {
        m_present_images[i].image = images[i];
    }

    m_present_format = surface_format.format;
    m_depth_format = find_depth_format(device.gpu());
    m_extent = extent;

    if (!create_image_views())
    {
        cleanup();
        return false;
    }

    if (!create_depth_resources(device.gpu()))
    {
        cleanup();
        return false;
    }

    return true;
}

void Swapchain::cleanup() noexcept
{
    for (size_t i = 0; i < m_present_images.size(); ++i)
    {
        vkDestroyImageView(m_device, m_present_images[i].image_view, nullptr);
        m_present_images[i].image_view = VK_NULL_HANDLE;
    }
    m_present_images.clear();

    vkDestroyImageView(m_device, m_depth_image_view, nullptr);
    m_depth_image_view = VK_NULL_HANDLE;

    vkDestroyImage(m_device, m_depth_image, nullptr);
    m_depth_image = VK_NULL_HANDLE;

    vkFreeMemory(m_device, m_depth_memory, nullptr);
    m_depth_memory = VK_NULL_HANDLE;

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

VkFormat Swapchain::find_supported_format(VkPhysicalDevice gpu, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags feature_flag) noexcept
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &format_properties);

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (format_properties.optimalTilingFeatures & feature_flag) == feature_flag)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat Swapchain::find_depth_format(VkPhysicalDevice gpu) noexcept
{
    return find_supported_format(gpu, 
                                 {
                                    VK_FORMAT_D32_SFLOAT,
                                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                                    VK_FORMAT_D24_UNORM_S8_UINT
                                 }, 
                                 VK_IMAGE_TILING_OPTIMAL, 
                                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSurfaceFormatKHR Swapchain::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats, VkFormat desired_format) noexcept
{
    for (const auto &available_format : available_formats)
    {
        if (available_format.format == desired_format && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR Swapchain::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_modes, VkPresentModeKHR desired_mode) noexcept
{
    for (const auto &avaiable_mode : available_modes)
    {
        if (avaiable_mode == desired_mode)
        {
            return avaiable_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::choose_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities, VkExtent2D actual_extent) noexcept
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    actual_extent.width = std::clamp(actual_extent.width,
                                     capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width);
    
    actual_extent.height = std::clamp(actual_extent.height, 
                                      capabilities.minImageExtent.height,
                                      capabilities.maxImageExtent.height);

    return actual_extent;
}

VkSwapchainKHR Swapchain::swapchain() const noexcept
{
    return m_swapchain;
}

VkExtent2D Swapchain::extent() const noexcept
{
    return m_extent;
}

VkFormat Swapchain::present_format() const noexcept
{
    return m_present_format;
}

VkFormat Swapchain::depth_format() const noexcept
{
    return m_depth_format;
}

VkImageView Swapchain::depth_image_view() const noexcept
{
    return m_depth_image_view;
}

const std::vector<Image> &Swapchain::present_images() const noexcept
{
    return m_present_images;
}

bool Swapchain::create_image_views() noexcept
{
    for (size_t i = 0; i < m_present_images.size(); ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = m_present_images[i].image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = m_present_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &create_info, nullptr, &m_present_images[i].image_view) != VK_SUCCESS)
        {
            LOG_ERROR("Swapchain", "Failed to create swapchain image views");

            for (size_t j = 0; j < i; ++j)
            {
                vkDestroyImageView(m_device, m_present_images[j].image_view, nullptr);
            }

            return false;
        }
    }

    LOG_INFO("Swapchain", "Swapchain image views created");

    return true; 
}

bool Swapchain::create_image(VkPhysicalDevice gpu,
                             uint32_t width,
                             uint32_t height,
                             VkFormat format,
                             VkImageTiling tiling,
                             VkImageUsageFlags usage_flags,
                             VkMemoryPropertyFlags property_flags,
                             VkImage &image,
                             VkDeviceMemory &image_memory) noexcept
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage_flags;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &image_info, nullptr, &image) != VK_SUCCESS)
    {
        LOG_ERROR("Swapchain", "Failed to create image");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(m_device, image, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(gpu, memory_requirements.memoryTypeBits, property_flags);

    if (alloc_info.memoryTypeIndex == UINT32_MAX)
    {
        LOG_ERROR("Swapchain", "No suitable memory type found");
        return false;
    }

    if (vkAllocateMemory(m_device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
    {
        LOG_ERROR("Swapchain", "Failed to allocate image memory");
        return false;
    }
    
    vkBindImageMemory(m_device, image, image_memory, 0);

    return true;
}

bool Swapchain::create_depth_resources(VkPhysicalDevice gpu) noexcept
{
    if (!create_image(gpu,
                      m_extent.width, 
                      m_extent.height, 
                      m_depth_format, 
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      m_depth_image,
                      m_depth_memory))
    {
        return false;
    }

    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (m_depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_depth_format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    m_depth_image_view = create_image_view(m_device, 
                                           m_depth_image, 
                                           m_depth_format, 
                                           aspect_flags);

    if (m_depth_image_view == VK_NULL_HANDLE)
    {
        LOG_ERROR("Swapchain", "Failed to create depth image view");
        return false;
    }

    return true;
}
} // namespace graphics
} // namespace niqqa