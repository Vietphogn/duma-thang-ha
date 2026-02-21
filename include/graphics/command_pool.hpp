#pragma once

#include <graphics/device.hpp>

#include <vulkan/vulkan.h>
#include <cstdint>

namespace niqqa
{
namespace graphics
{
class CommandPool 
{
public:
    bool init(VkDevice device, uint32_t queue_family_index, VkCommandPoolCreateFlags flags) noexcept;
    void cleanup() noexcept;

    VkCommandBuffer allocate_primary() noexcept;
    VkCommandBuffer allocate_secondary() noexcept;

    void reset() noexcept;

    VkCommandPool pool() const noexcept;

private:
    VkDevice m_device{VK_NULL_HANDLE};
    VkCommandPool m_pool{VK_NULL_HANDLE};
};
} // namespace graphics
} // namespace niqqa