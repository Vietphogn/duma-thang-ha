#pragma once

#include <graphics/frame.hpp>
#include <graphics/device.hpp>
#include <graphics/render_pass.hpp>
#include <graphics/swapchain.hpp>

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace niqqa
{
namespace systems
{
class ForwardRenderer final
{
public:
    bool init(graphics::Device *device, graphics::Swapchain *swapchain) noexcept;
    void draw_frame() noexcept;
    void resize() noexcept;
    void cleanup() noexcept;

private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{2};

    uint32_t m_frame_index{0};
    std::vector<graphics::Frame> m_frames;

    graphics::Device *m_device{nullptr};
    graphics::Swapchain *m_swapchain{nullptr};

    graphics::RenderPass m_render_pass;

    void record_commands(VkCommandBuffer command_buffer, uint32_t image_index) noexcept;
};
} // namespace systems
} // namespace niqqa