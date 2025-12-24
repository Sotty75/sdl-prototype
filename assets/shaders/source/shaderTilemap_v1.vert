#version 450 core

struct tilesetInfo {
    int COLUMNS;
    int TILE_W;
    int TILE_H;
    int TILESET_W;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) out vec2 texCoord;

layout (set = 1, binding = 0) uniform Tileset {
    tilesetInfo ti;
};
layout (set = 1, binding = 1) uniform Camera {
    mat4 projection_view;
};

void main()
{
    // calculate K & R
    float k = gl_InstanceIndex % ti.COLUMNS;
    float r = gl_InstanceIndex / ti.COLUMNS;

    // calculate Translation Matrix
    mat4 model = mat4(1.0);
    model[3][0] = k;
    model[3][1] = r;

    texCoord = inTexCoord;
    gl_Position = projection_view * model[gl_InstanceIndex] * vec4(inPos, 1.0);
}
