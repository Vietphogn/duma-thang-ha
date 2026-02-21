#include <graphics/command_pool.hpp>

#include <log.hpp>

namespace niqqa
{
namespace graphics
{
bool CommandPool::init(VkDevice device, uint32_t queue_family_index, VkCommandPoolCreateFlags flags) noexcept
{
    m_device = device;

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = flags;
    pool_info.queueFamilyIndex = queue_family_index;

    if (vkCreateCommandPool(device, &pool_info, nullptr, &m_pool) != VK_SUCCESS)
    {
        LOG_ERROR("Command Pool", "Failed to create command pool");
        return false;
    }

    return true;
}

void CommandPool::cleanup() noexcept
{
    vkDestroyCommandPool(m_device, m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
}

VkCommandBuffer CommandPool::allocate_primary() noexcept
{
    VkCommandBuffer command_buffer;

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);

    return command_buffer;
}

VkCommandBuffer CommandPool::allocate_secondary() noexcept
{
    VkCommandBuffer command_buffer;

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);

    return command_buffer;
}

void CommandPool::reset() noexcept
{
    vkResetCommandPool(m_device, m_pool, 0);
}

VkCommandPool CommandPool::pool() const noexcept
{
    return m_pool;
}
} // namespace graphics
} // namespace niqqan