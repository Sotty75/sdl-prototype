#version 450 core

// Per-instance sprite data, read from the storage buffer (SSBO).
// Must match the layout of SOT_GPU_SpriteInstance on the CPU side.
struct SpriteInfo {
    vec2 POSITION;
    ivec2 FRAME_POS;
    ivec2 FRAME_SIZE;
    ivec2 ATLAS_SIZE;
    uint ATLAS_INDEX;
    uint FLIP_FLAGS;    // bit 0 = horizontal flip, bit 1 = vertical flip
    vec4 TINT_COLOR;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec2 outTexCoord;
layout (location = 1) flat out vec4 spriteUVBounds;
layout (location = 2) flat out uint atlasIndex;
layout (location = 3) flat out vec4 tintColor;

layout (set = 0, binding = 0) buffer Sprites {
    SpriteInfo sprites[];
};

layout (set = 1, binding = 0) uniform Camera {
    mat4 projection_view;
};

void main()
{
    SpriteInfo sprite = sprites[gl_InstanceIndex];

    // Model matrix: scale to frame size and translate to position
    mat4 model = mat4(1.0);
    model[0][0] = sprite.FRAME_SIZE.x;
    model[1][1] = sprite.FRAME_SIZE.y;
    model[3][0] = sprite.POSITION.x;
    model[3][1] = sprite.POSITION.y;

    // UV calculation
    float spriteUVWidth  = float(sprite.FRAME_SIZE.x) / float(sprite.ATLAS_SIZE.x);
    float spriteUVHeight = float(sprite.FRAME_SIZE.y) / float(sprite.ATLAS_SIZE.y);
    vec2 uvOffset = vec2(sprite.FRAME_POS.x / sprite.ATLAS_SIZE.x, sprite.FRAME_POS.y / sprite.ATLAS_SIZE.y);

    // Half-texel inset to prevent atlas bleeding
    vec2 texelSize = vec2(1.0) / vec2(sprite.ATLAS_SIZE.x, sprite.ATLAS_SIZE.y);
    vec2 inset = texelSize * 0.5;
    vec2 uvMin = uvOffset + inset;
    vec2 uvMax = uvOffset + vec2(spriteUVWidth, spriteUVHeight) - inset;
    spriteUVBounds = vec4(uvMin, uvMax);

    // Apply flip to texture coordinates
    vec2 tc = inTexCoord;
    if ((sprite.FLIP_FLAGS & 1u) != 0u) tc.x = 1.0 - tc.x;  // Horizontal flip
    if ((sprite.FLIP_FLAGS & 2u) != 0u) tc.y = 1.0 - tc.y;  // Vertical flip

    atlasIndex = sprite.ATLAS_INDEX;
    tintColor = sprite.TINT_COLOR;
    outTexCoord = uvOffset + (tc * vec2(spriteUVWidth, spriteUVHeight));
    gl_Position = projection_view * model * vec4(inPos, 1.0);
}
