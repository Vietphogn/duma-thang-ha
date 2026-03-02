#include <systems/renderers/forward.hpp>

#include <graphics/image.hpp>
#include <log.hpp>

namespace niqqa
{
namespace systems
{
bool ForwardRenderer::init(graphics::Device *device, 
                           graphics::Swapchain *swapchain, 
                           const std::string &vert_path, 
                           const std::string &frag_path) noexcept
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

    if (!m_render_pass.create(m_device->device(),
                              m_swapchain->present_format(), 
                              m_swapchain->depth_format()))
    {
        return false;
    }

    if (!m_pipeline.create(m_device->device(), swapchain->extent(), m_render_pass.render_pass(), vert_path, frag_path))
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
                          VK_NULL_HANDLE, 
                          &image_index);

    current_frame.begin_commands();
}

// move this shit to swapchain
bool ForwardRenderer::create_framebuffers(VkRenderPass render_pass) noexcept
{
    std::vector<graphics::Image> present_images = m_swapchain->present_images();
    VkImageView depth_image_view = m_swapchain->depth_image_view();
    VkExtent2D extent = m_swapchain->extent();

    m_framebuffers.resize(present_images.size());

    for (size_t i = 0; i < m_framebuffers.size(); ++i)
    {
        VkImageView attachments[] = {
            present_images[i].image_view,
            depth_image_view 
        };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(m_device->device(), &framebuffer_info, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
        {
            LOG_ERROR("Swapchain", "Failed to create framebuffer");

            for (size_t j = 0; j < i; ++j)
            {
                vkDestroyFramebuffer(m_device->device(), m_framebuffers[j], nullptr);
            }

            return false;
        }
    }

    return true;
}

void ForwardRenderer::record_commands(VkCommandBuffer command_buffer, uint32_t image_index) noexcept
{
    VkClearValue clear_values[2]{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = m_render_pass.render_pass();
    begin_info.framebuffer = m_framebuffers[image_index];
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
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline());

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
}
} // namespace systems
} // namespace niqqa