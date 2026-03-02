#include <graphics/descriptor_writer.hpp>

namespace niqqa
{
namespace graphics
{
DescriptorWriter &DescriptorWriter::begin(VkDescriptorSet dst_set) noexcept
{
    m_dst_set = dst_set;

    m_writes.clear();
    m_buffer_infos.clear();
    m_image_infos.clear();

    m_writes.reserve(8);
    m_buffer_infos.reserve(8);
    m_image_infos.reserve(8);

    return *this;
}

DescriptorWriter &DescriptorWriter::write_buffer(uint32_t binding,
                                                 VkBuffer buffer,
                                                 VkDeviceSize size,
                                                 VkDescriptorType type) noexcept
{
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer;
    buffer_info.offset = 0;
    buffer_info.range = size;
    
    m_buffer_infos.push_back(std::move(buffer_info));

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = m_dst_set;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &m_buffer_infos.back();

    m_writes.push_back(std::move(write));

    return *this;
}
DescriptorWriter &DescriptorWriter::write_image(uint32_t binding,
                                                VkImageView image_view,
                                                VkSampler sampler,
                                                VkDescriptorType type) noexcept
{
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = image_view;
    image_info.sampler = sampler;

    m_image_infos.push_back(std::move(image_info));

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = m_dst_set;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &m_image_infos.back();

    m_writes.push_back(std::move(write));

    return *this;
}

void DescriptorWriter::update(VkDevice device) noexcept
{
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(m_writes.size()),
                           m_writes.data(), 
                           0, 
                           nullptr);
}
} // namespace graphics
} // namespace niqqa2