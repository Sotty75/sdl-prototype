#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) out vec2 texCoord;

layout (set = 1, binding = 0) uniform Model {
    mat4 model;
};
layout (set = 1, binding = 1) uniform Camera {
    mat4 projection_view;
};

void main()
{
    texCoord = inTexCoord;
    gl_Position = projection_view * model * vec4(inPos, 1.0);
}
