#version 450 core

struct TilemapInfo {
    int COLUMNS;
    int ROWS;
    int TILE_W;
    int TILE_H;
    int TILESET_W;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) out vec2 texCoord;

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
    float k = gl_InstanceIndex % ti.COLUMNS;
    float r = gl_InstanceIndex / ti.COLUMNS;

    // calculate Translation Matrix
    mat4 model = mat4(1.0);
    model[3][0] = k;
    model[3][1] = -r + ti.ROWS;

    // --- 2. UV Logic (The Fix) ---
    
    // Get the ID of the tile we want to draw
    int index = tiles[gl_InstanceIndex]-1;

    // How many tiles fit in the texture width? (e.g., 256 / 32 = 8 cols)
    int tilesPerRow = ti.TILESET_W / ti.TILE_W;

    // Calculate which grid cell in the texture atlas this tile belongs to
    // e.g. Index 10 -> Column 2, Row 1
    float col = float(index % tilesPerRow); 
    float row = float(index / tilesPerRow);

    // Calculate the size of ONE tile in UV space (0.0 to 1.0)
    // e.g. 32 / 256 = 0.125
    float tileUVWidth  = float(ti.TILE_W) / float(ti.TILESET_W);
    
    // NOTE: If your texture is square, use TILESET_W for height too.
    // If not, you need to add TILESET_H to your struct.
    float tileUVHeight = float(ti.TILE_H) / float(ti.TILESET_W); 

    // Calculate the Base UV (Top-Left corner of the specific tile in the atlas)
    vec2 uvOffset = vec2(col * tileUVWidth, row * tileUVHeight);

    // Calculate Final UVs
    // We take the generic 0..1 UVs from the mesh (inTexCoord)
    // Scale them down to the size of one tile
    // And Add the offset to move them to the correct spot in the atlas
    // texCoord = uvOffset + (inTexCoord * vec2(tileUVWidth, tileUVHeight));

    // Original UV calculation
    // texCoord = uvOffset + (inTexCoord * vec2(tileUVWidth, tileUVHeight));

    // --- THE FIX: UV Inset ---
    // We shrink the sampling area by a tiny amount (half a pixel) to stay safe.
    // 1. Calculate the size of a single pixel in UV space
    vec2 pixelSize = vec2(1.0) / vec2(ti.TILESET_W, ti.TILESET_W); // Assuming square texture
    
    // 2. Define a small inset (0.5 pixels is usually enough)
    vec2 halfPixel = pixelSize * 0.5;

    // 3. Calculate the Min and Max UVs for this specific tile
    vec2 uvMin = uvOffset + halfPixel;
    vec2 uvMax = uvOffset + vec2(tileUVWidth, tileUVHeight) - halfPixel;

    // 4. Map the input (0..1) to this safe range
    texCoord = mix(uvMin, uvMax, inTexCoord);
    gl_Position = projection_view * model * vec4(inPos, 1.0);
}
