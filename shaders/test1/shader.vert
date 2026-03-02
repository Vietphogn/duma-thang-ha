#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_uv;

layout (location = 0) out vec3 frag_color;
layout (location = 1) out vec2 frag_uv;

layout (set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform PushConstant
{
    mat4 model;
    vec4 color;
} pc;

void main() 
{
    mat4 mvp = ubo.proj * ubo.view * pc.model;

    gl_Position = mvp * vec4(in_pos, 1.0);

    frag_color = in_color;
    frag_uv = in_uv;
}