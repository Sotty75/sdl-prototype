#version 450 core

layout (location = 3) in vec2 texCoord;
layout (location = 0) out vec4 FragColor;
layout (set = 2, binding = 0) uniform sampler2D texture_1;
layout (set = 2, binding = 1) uniform sampler2D texture_2;

void main()
{
    vec4 texColor1 = texture(texture_1, texCoord);
    vec4 texColor2 = texture(texture_2, texCoord);
    
    FragColor = vec4(texColor1); 
} 

