#version 450 core

struct SpriteInfo {
    vec2 POSITION;        
    ivec2 FRAME_POS;      
    ivec2 FRAME_SIZE;
    ivec2 ATLAS_SIZE;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec2 outTexCoord;
layout (location = 1) flat out vec4 spriteUVBounds;

layout (set = 0, binding = 0) buffer Sprites {
    SpriteInfo sprites[];
};
layout (set = 1, binding = 0) uniform Camera {
    mat4 projection_view;
};

void main()
{
    // Get the current sprite instance
    SpriteInfo sprite = sprites[gl_InstanceIndex];

    // Scale the vertices of the base quad to adjust them to the 
    // Calculate the translation factor of each sprite
    mat4 model = mat4(1.0);
    model[0][0] = sprite.FRAME_SIZE.x;
    model[1][1] = sprite.FRAME_SIZE.y;
    model[3][0] = sprite.POSITION.x;
    model[3][0] = sprite.POSITION.y;

    // 1. Calculate Sprite SDimensions
    float spriteUVWidth  = float(sprite.FRAME_SIZE.x) / float(sprite.ATLAS_SIZE.x);
    float spriteUVHeight = float(sprite.FRAME_SIZE.y) / float(sprite.ATLAS_SIZE.y);
    vec2 uvOffset = vec2(sprite.FRAME_POS.x / sprite.ATLAS_SIZE.x, sprite.FRAME_POS.y / sprite.ATLAS_SIZE.y);

    // 3. THE FIX: Calculate Safe Bounds (Inset by 0.5 texels)
    // FIX: Use TILESET_HEIGHT for Y axis to prevent vertical bleeding
    vec2 texelSize = vec2(1.0) / vec2(sprite.ATLAS_SIZE.x, sprite.ATLAS_SIZE.y); 
    vec2 inset = texelSize * 0.5; // Move to the center of the edge pixel

    vec2 uvMin = uvOffset + inset;
    vec2 uvMax = uvOffset + vec2(spriteUVWidth, spriteUVHeight) - inset;

    // Pass the HARD LIMITS to the fragment shader
    spriteUVBounds = vec4(uvMin, uvMax);

    // Pass the standard interpolated coordinate
    // (We don't need mix() here anymore, the fragment shader handles the safety)
    texCoord = uvOffset + (inTexCoord * vec2(spriteUVWidth, spriteUVHeight));
    gl_Position = projection_view * model * vec4(inPos, 1.0);
}
