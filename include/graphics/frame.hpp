#pragma once

#include <graphics/command_pool.hpp>

#include <vulkan/vulkan.h>
#include <cstdint>

namespace niqqa
{
namespace graphics
{
struct Frame 
{
    // command
    CommandPool command_pool;
    VkCommandBuffer command_buffer{VK_NULL_HANDLE};

    // sync objects
    VkSemaphore acquire_semaphore{VK_NULL_HANDLE};
    VkSemaphore present_semaphore{VK_NULL_HANDLE};
    VkFence frame_fence{VK_NULL_HANDLE};

    // resources
    VkBuffer uniform_buffer{VK_NULL_HANDLE};
    VkDeviceMemory uniform_memory{VK_NULL_HANDLE};
    
    VkDescriptorSet descriptor_set{VK_NULL_HANDLE};

    bool init(VkDevice device, uint32_t queue_family_index) noexcept;
    void destroy(VkDevice device) noexcept;

    void wait_and_reset(VkDevice device) noexcept;

    void begin_commands() noexcept;
    void end_commands() noexcept;
};
} // namespace graphics
} // namespace niqqa