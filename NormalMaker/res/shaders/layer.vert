#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 ortho;
} ubo;

layout(push_constant) uniform constants {
    ivec2 position;
    ivec2 size;
    ivec2 canvasSize;

    float zOff;
    float padding;
    float alpha;
} PushConstants;

layout(location = 0) out vec2 v_Pos;
layout(location = 1) out vec2 v_UV;

void main()
{
    v_Pos = position * PushConstants.size + PushConstants.position;

    gl_Position = ubo.ortho * ubo.view * vec4(v_Pos, PushConstants.zOff, 1.0);

    v_UV = uv;
}
