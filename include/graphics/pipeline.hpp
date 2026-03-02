#pragma once

#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <string>

namespace niqqa 
{
struct PipelineConfig 
{
    VkRenderPass render_pass{VK_NULL_HANDLE};
    VkExtent2D extent;

    VkBool32 depth_test{VK_FALSE};
    VkBool32 enable_blending{VK_FALSE};

    VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkCullModeFlags cull_mode{VK_CULL_MODE_BACK_BIT};
};

struct alignas(16) VertexInput 
{
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec2 uv;
};

struct alignas(16) MeshPushConstants 
{
    glm::mat4 model;
    glm::vec4 color;
};

namespace graphics
{
class GraphicsPipeline 
{
public:
    bool create(VkDevice device, 
                VkExtent2D extent, 
                VkRenderPass render_pass, 
                const std::string &vert_path, 
                const std::string &frag_path) noexcept;
    void cleanup() noexcept;

    VkPipeline pipeline() const noexcept;

private:
    VkDevice m_device{VK_NULL_HANDLE};

    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};

    VkPipelineCache m_cache{VK_NULL_HANDLE};

    VkDescriptorSetLayout m_descriptor_set_layout{VK_NULL_HANDLE};

    bool create_descriptor_set_layout() noexcept;
    bool create_pipeline_layout() noexcept;
    bool create_cache() noexcept;
    bool create_graphics_pipeline(PipelineConfig config, 
                                 const std::string &vert_path, 
                                 const std::string &frag_path) noexcept;

};
} // namespace graphics
} // namespace niqqa