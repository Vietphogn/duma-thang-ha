#include <graphics/pipeline.hpp>

#include <log.hpp>

#include <vector>
#include <fstream>
#include <cstdint>
#include <cstddef>
#include <string>
#include <format>

namespace niqqa 
{
namespace graphics
{
static std::vector<char> read_file(const std::string &filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("Pipeline", "Failed to open file: " + filename);
        return {};
    }

    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);

    file.seekg(0);
    file.read(buffer.data(), size);

    return buffer;
}

static VkShaderModule create_shader_module(VkDevice device, const std::vector<char> &code)
{
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = code.size();
    shader_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &shader_info, nullptr, &module) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return module;
}

bool GraphicsPipeline::create(VkDevice device, 
                              VkExtent2D extent, 
                              VkRenderPass render_pass, 
                              const std::string &vert_path, 
                              const std::string &frag_path) noexcept
{
    m_device = device;

    if (!create_descriptor_set_layout())
    {
        return false;
    }

    if (!create_pipeline_layout())
    {
        return false;
    }

    if (!create_cache())
    {
        return false;
    }

    PipelineConfig config{};
    config.render_pass = render_pass;
    config.extent = extent;
    config.depth_test = VK_TRUE;
    config.enable_blending = VK_FALSE;
    config.cull_mode = VK_CULL_MODE_BACK_BIT;
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    if (!create_graphics_pipeline(config, vert_path, frag_path))
    {
        return false;
    }

    return true;
}

void GraphicsPipeline::cleanup() noexcept
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;

    vkDestroyPipelineCache(m_device, m_cache, nullptr);
    m_cache = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
    m_pipeline_layout = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
    m_descriptor_set_layout = VK_NULL_HANDLE;
}

VkPipeline GraphicsPipeline::pipeline() const noexcept
{
    return m_pipeline;
}

bool GraphicsPipeline::create_descriptor_set_layout() noexcept
{
    VkDescriptorSetLayoutBinding ubo_binding{};
    ubo_binding.binding = 0;
    ubo_binding.descriptorCount = 1;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding sampled_image_binding{};
    sampled_image_binding.binding = 1;
    sampled_image_binding.descriptorCount = 1;
    sampled_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampled_image_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampled_image_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = {
        ubo_binding,
        sampled_image_binding
    };
    
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;
    layout_info.flags = 0;

    if (vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descriptor_set_layout) != VK_SUCCESS)
    {
        LOG_ERROR("Pipeline", "Failed to create descriptor set layout");
        return false;
    }

    return true;
}

bool GraphicsPipeline::create_pipeline_layout() noexcept
{
    VkPushConstantRange push_constants{};
    push_constants.offset = 0;
    push_constants.size = sizeof(MeshPushConstants);
    push_constants.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &m_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constants;

    if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
    {
        LOG_ERROR("Pipeline", "Failed to create pipeline layout");
        return false;
    }

    return true;
}

bool GraphicsPipeline::create_cache() noexcept
{
    VkPipelineCacheCreateInfo cache_info{};
    cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(m_device, &cache_info, nullptr, &m_cache) != VK_SUCCESS)
    {
        LOG_ERROR("Pipeline", "Failed to create pipeline cache");
        return false;
    }

    return true;
}

bool GraphicsPipeline::create_graphics_pipeline(PipelineConfig config, 
                                               const std::string &vert_path, 
                                               const std::string &frag_path) noexcept
{
    std::vector<char> vert_code = read_file(vert_path);

    if (vert_code.empty())
    {
        std::string msg = std::format("Failed to read file: {}", vert_path);
        LOG_ERROR("Pipeline", msg);

        return false;
    }
    
    std::vector<char> frag_code = read_file(frag_path);

    if (frag_code.empty())
    {
        std::string msg = std::format("Failed to read file: {}", frag_path);
        LOG_ERROR("Pipeline", msg);

        return false;
    }

    VkShaderModule vert_module = create_shader_module(m_device, vert_code);
    VkShaderModule frag_module = create_shader_module(m_device, frag_code);
    
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
    {
        LOG_ERROR("Pipeline", "Failed to create shader module");
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_module;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_module;
    stages[1].pName = "main";

    VkVertexInputBindingDescription vertex_input_binding{};
    vertex_input_binding.binding = 0;
    vertex_input_binding.stride = sizeof(VertexInput);
    vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attributes[3]{};

    vertex_input_attributes[0].binding = 0;
    vertex_input_attributes[0].location = 0;
    vertex_input_attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attributes[0].offset = offsetof(VertexInput, pos);

    vertex_input_attributes[1].binding = 0;
    vertex_input_attributes[1].location = 1;
    vertex_input_attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attributes[1].offset = offsetof(VertexInput, color);

    vertex_input_attributes[2].binding = 0;
    vertex_input_attributes[2].location = 2;
    vertex_input_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attributes[2].offset = offsetof(VertexInput, uv);

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding;
    vertex_input_info.vertexAttributeDescriptionCount = 3;
    vertex_input_info.pVertexAttributeDescriptions = vertex_input_attributes;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = config.topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = nullptr;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = nullptr;

    VkDynamicState dynamic_viewport[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_viewport;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = config.cull_mode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = config.depth_test;
    depth_stencil.depthWriteEnable = config.depth_test;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    // Color blend
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT | 
                                            VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = config.enable_blending;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.logicOpEnable = VK_FALSE;

    // Pipeline info
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;

    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;

    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = config.render_pass;
    pipeline_info.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device, m_cache, 1, &pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        LOG_ERROR("Pipeline", "Failed to create graphics pipeline");

        vkDestroyShaderModule(m_device, vert_module, nullptr);
        vkDestroyShaderModule(m_device, frag_module, nullptr);

        return false;
    }

    vkDestroyShaderModule(m_device, vert_module, nullptr);
    vkDestroyShaderModule(m_device, frag_module, nullptr);

    return true;
}
} // namespace graphics
} // namespace niqqa