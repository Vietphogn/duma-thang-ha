#pragma once

#include <graphics/frame.hpp>
#include <graphics/device.hpp>
#include <graphics/render_pass.hpp>
#include <graphics/swapchain.hpp>
#include <graphics/pipeline.hpp>

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <string>

namespace niqqa
{
namespace systems
{
class ForwardRenderer final
{
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{2};

    bool init(graphics::Device *device, 
              graphics::Swapchain *swapchain, 
              const std::string &vert_path, 
              const std::string &frag_path) noexcept;
    void draw_frame() noexcept;
    void resize() noexcept;
    void cleanup() noexcept;

private:
    uint32_t m_frame_index{0};

    std::vector<graphics::Frame> m_frames;
    std::vector<VkFramebuffer> m_framebuffers;

    graphics::Device *m_device{nullptr};
    graphics::Swapchain *m_swapchain{nullptr};
    graphics::RenderPass m_render_pass;
    graphics::GraphicsPipeline m_pipeline;

    bool create_framebuffers(VkRenderPass render_pass) noexcept;

    void record_commands(VkCommandBuffer command_buffer, uint32_t image_index) noexcept;
};
} // namespace systems
} // namespace niqqa