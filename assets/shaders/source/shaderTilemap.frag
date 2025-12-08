#version 450 core

layout (location = 3) in vec2 texCoord;
layout (location = 0) out vec4 FragColor;
layout (set = 2, binding = 0) uniform sampler2D tileset;

void main()
{
    vec4 texColor = texture(tileset, texCoord);
    FragColor = vec4(texColor); 
} 

