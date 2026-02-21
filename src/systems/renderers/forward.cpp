#include <systems/renderers/forward.hpp>

namespace niqqa
{
namespace systems
{
bool ForwardRenderer::init(graphics::Device *device, graphics::Swapchain *swapchain) noexcept
{
    m_device = device;
    m_swapchain = swapchain;

    m_frames.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (!m_frames[i].init(m_device->device(), m_device->graphics_queue_family()))
        {
            return false;
        }
    }

    if (!m_render_pass.init(m_device->device(), m_swapchain->present_format(), m_swapchain->depth_format()))
    {
        return false;
    }

    return true;
}

void ForwardRenderer::draw_frame() noexcept
{
    graphics::Frame &current_frame = m_frames[m_frame_index];

    current_frame.wait_and_reset(m_device->device());

    uint32_t image_index;

    vkAcquireNextImageKHR(m_device->device(), 
                          m_swapchain->swapchain(), 
                          UINT64_MAX, 
                          current_frame.acquire_semaphore, 
                          current_frame.frame_fence, 
                          &image_index);

    current_frame.begin_commands();
}

void ForwardRenderer::record_commands(VkCommandBuffer command_buffer, uint32_t image_index) noexcept
{
    VkClearValue clear_values[2]{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = m_render_pass.render_pass();
    begin_info.framebuffer = m_swapchain->framebuffers(image_index);
    begin_info.clearValueCount = 2;
    begin_info.pClearValues = clear_values;
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent = m_swapchain->extent();

    vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = static_cast<float>(m_swapchain->extent().width);
    viewport.height = static_cast<float>(m_swapchain->extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->extent();

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // TODO: add graphics pipeline
}
} // namespace systems
} // namespace niqqa