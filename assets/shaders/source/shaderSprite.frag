#version 450 core

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) flat in vec4 spriteUVBounds;
layout (location = 2) flat in uint atlasIndex;
layout (location = 3) flat in vec4 tintColor;
layout (location = 0) out vec4 fragColor;
layout (set = 2, binding = 0) uniform sampler2D atlas[16];

void main()
{
    vec2 safeUV = clamp(inTexCoord, spriteUVBounds.xy, spriteUVBounds.zw);
    vec4 texColor = texture(atlas[atlasIndex], safeUV);
    fragColor = texColor * tintColor;
}
