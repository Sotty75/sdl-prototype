#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) out vec2 texCoord;

layout (set = 0, binding = 0) buffer Transform {
    mat4 model[];
};
layout (set = 1, binding = 0) uniform Camera {
    mat4 projection_view;
};

void main()
{
    texCoord = inTexCoord;
    gl_Position = projection_view * model[gl_InstanceIndex] * vec4(inPos, 1.0);
}
