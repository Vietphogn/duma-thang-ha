#pragma once

#include <vulkan/vulkan.h>

namespace niqqa
{
namespace graphics
{
class RenderPass 
{
public:
    bool create(VkDevice device, VkFormat color_format, VkFormat depth_format) noexcept;
    void cleanup(VkDevice device) noexcept;

    VkRenderPass render_pass() const noexcept;

private:
    VkRenderPass m_render_pass{VK_NULL_HANDLE};
};
} // namespace graphics
} // namespace niqqa