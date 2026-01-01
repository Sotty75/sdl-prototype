#version 450 core

struct TilemapInfo {
    int COLUMNS;
    int ROWS;
    int TILE_W;
    int TILE_H;
    int TILESET_W;
    int TILESET_H;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec2 texCoord;
layout (location = 1) flat out vec4 tileUVBounds;

layout (set = 0, binding = 0) buffer TilemapData {
    int tiles[];
};
layout (set = 1, binding = 0) uniform Camera {
    mat4 projection_view;
};
layout (set = 1, binding = 1) uniform Tilemap {
    TilemapInfo ti;
};


void main()
{
    // calculate K & R
    // adding 0.5 as the original quad is centered on the origin with side = 1.
    float k = gl_InstanceIndex % ti.COLUMNS + 0.5;
    float r = gl_InstanceIndex / ti.COLUMNS + 0.5;

    // calculate Translation Matrix
    mat4 model = mat4(1.0);
    model[3][0] = k;
    model[3][1] = -r + ti.ROWS;

    // --- 2. UV Logic (The Fix) ---
    
    int tileID = tiles[gl_InstanceIndex];
    if (tileID == 0) { gl_Position = vec4(0.0); return; }
    int index = tileID - 1;

    // 1. Calculate Grid Position
    int tilesPerRow = ti.TILESET_W / ti.TILE_W;
    float col = float(index % tilesPerRow);
    float row = float(index / tilesPerRow);

    // 2. Calculate UV Dimensions
    float tileUVWidth  = float(ti.TILE_W) / float(ti.TILESET_W);
    float tileUVHeight = float(ti.TILE_H) / float(ti.TILESET_H);

    vec2 uvOffset = vec2(col * tileUVWidth, row * tileUVHeight);

    // 3. THE FIX: Calculate Safe Bounds (Inset by 0.5 texels)
    // FIX: Use TILESET_HEIGHT for Y axis to prevent vertical bleeding
    vec2 texelSize = vec2(1.0) / vec2(ti.TILESET_W, ti.TILESET_H); 
    vec2 inset = texelSize * 0.5; // Move to the center of the edge pixel

    vec2 uvMin = uvOffset + inset;
    vec2 uvMax = uvOffset + vec2(tileUVWidth, tileUVHeight) - inset;

    // Pass the HARD LIMITS to the fragment shader
    tileUVBounds = vec4(uvMin, uvMax);

    // Pass the standard interpolated coordinate
    // (We don't need mix() here anymore, the fragment shader handles the safety)
    texCoord = uvOffset + (inTexCoord * vec2(tileUVWidth, tileUVHeight));


    gl_Position = projection_view * model * vec4(inPos, 1.0);
}
