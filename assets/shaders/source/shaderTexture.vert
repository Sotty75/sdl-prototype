#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 0) out vec2 texCoord;

layout (set = 1, binding = 0) uniform Uniforms {
    mat4 transform;
};

void main()
{
    texCoord = inTexCoord;
    gl_Position = transform * vec4(inPos, 1.0);
}
