#include <graphics/frame.hpp>

#include <log.hpp>

namespace niqqa
{
namespace graphics
{
bool Frame::init(VkDevice device, uint32_t queue_family_index) noexcept
{
    if (!command_pool.init(device, 
                           queue_family_index,
                           VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
    {
        return false;
    }

    command_buffer = command_pool.allocate_primary();

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphore_info, nullptr, &acquire_semaphore) != VK_SUCCESS)
    {
        LOG_ERROR("Frame", "Failed to create acquire semaphore");
        destroy(device);

        return false;
    }
    
    if (vkCreateSemaphore(device, &semaphore_info, nullptr, &present_semaphore) != VK_SUCCESS)
    {
        LOG_ERROR("Frame", "Failed to create present semaphore");
        destroy(device);
        
        return false;
    }

    if (vkCreateFence(device, &fence_info, nullptr, &frame_fence) != VK_SUCCESS)
    {
        LOG_ERROR("Frame", "Failed to create frame fence");
        destroy(device);

        return false;
    }

    return true;
}

void Frame::destroy(VkDevice device) noexcept
{
    vkDestroyFence(device, frame_fence, nullptr);
    frame_fence = VK_NULL_HANDLE;

    vkDestroySemaphore(device, acquire_semaphore, nullptr);
    acquire_semaphore = VK_NULL_HANDLE;

    vkDestroySemaphore(device, present_semaphore, nullptr);
    present_semaphore = VK_NULL_HANDLE;

    command_pool.cleanup();
}

void Frame::wait_and_reset(VkDevice device) noexcept
{
    vkWaitForFences(device, 1, &frame_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frame_fence);

    command_pool.reset();
}

void Frame::begin_commands() noexcept
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void Frame::end_commands() noexcept
{
    vkEndCommandBuffer(command_buffer);
}
} // namespace graphics
} // namespace niqqa