#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 ortho;
} ubo;

layout(location = 0) out vec3 v_Color;

void main()
{
    gl_Position = ubo.ortho * ubo.view * vec4(position, 9.0, 1.0);

    v_Color = color;
}
