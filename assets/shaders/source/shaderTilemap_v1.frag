#version 450 core

layout (location = 0) in vec2 texCoord;
layout (location = 1) flat in vec4 tileUVBounds;
layout (location = 0) out vec4 fragColor;
layout (set = 2, binding = 0) uniform sampler2D tileset;

void main()
{
    vec2 safeUV = clamp(texCoord, tileUVBounds.xy, tileUVBounds.zw);
    fragColor = texture(tileset, safeUV);
} 

