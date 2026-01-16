#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec3 vertexColor;

layout (set = 1, binding = 0) uniform Camera {
    mat4 projection_view;
};

void main()
{
    vertexColor = inColor;
    gl_Position = projection_view * vec4(inPos, 1.0);
}
