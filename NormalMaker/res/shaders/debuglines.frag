#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
    float alpha;
} PushConstants;

layout(location = 0) in vec3 v_Color;

void main()
{
    outColor = vec4(v_Color, PushConstants.alpha);

    // Gamma corretion
    outColor = pow(outColor, vec4(vec3(1.0 / 2.2), 1.0));
}
