#version 450 core

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) flat in vec4 spriteUVBounds;
layout (location = 0) out vec4 fragColor;
layout (set = 2, binding = 0) uniform sampler2D atlas;

void main()
{
    vec2 safeUV = clamp(texCoord, spriteUVBounds.xy, spriteUVBounds.zw);
    fragColor = texture(atlas, safeUV);
} 

