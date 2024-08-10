#version 450

#define MAX_LAYERS 16

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D layer;

layout(push_constant) uniform constants {
    ivec2 position;
    ivec2 size;
    ivec2 canvasSize;

    float zOff;
    float padding;
    float alpha;
} PushConstants;

layout(location = 0) in vec2 v_Pos;
layout(location = 1) in vec2 v_UV;

void main()
{
    if(any(greaterThanEqual(v_Pos, PushConstants.canvasSize)) ||
        any(lessThan(v_Pos, vec2(0.0))))
        discard;

    outColor = texture(layer, v_UV);
    outColor.a *= PushConstants.alpha;

    if(outColor.a == 0)
        discard;
}
