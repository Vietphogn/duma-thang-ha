#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace niqqa
{
namespace graphics
{
class DescriptorWriter
{
public:
    DescriptorWriter &begin(VkDescriptorSet dst_set) noexcept;

    DescriptorWriter &write_buffer(uint32_t binding,
                                   VkBuffer buffer,
                                   VkDeviceSize size,
                                   VkDescriptorType type) noexcept;

    DescriptorWriter &write_image(uint32_t binding,
                                   VkImageView image_view,
                                   VkSampler sampler,
                                   VkDescriptorType type) noexcept;

    void update(VkDevice device) noexcept;

private:
    VkDescriptorSet m_dst_set{VK_NULL_HANDLE};

    std::vector<VkWriteDescriptorSet> m_writes;
    std::vector<VkDescriptorBufferInfo> m_buffer_infos;
    std::vector<VkDescriptorImageInfo> m_image_infos;
};
} // namespace graphics
} // namespace niqqa