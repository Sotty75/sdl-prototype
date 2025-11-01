#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec3 vertexColor;
layout (location = 1) out vec2 texCoord;

void main()
{
    vertexColor = inColor;
    texCoord = inTexCoord;
    gl_Position = vec4(inPos.x, inPos.y, inPos.z, 1.0);
}