#version 450 core

layout (location = 0) in vec2 texCoord;
layout (location = 0) out vec4 fragColor;
layout (set = 2, binding = 0) uniform sampler2D texSampler;

void main()
{
    vec4 texColor = texture(texSampler, texCoord);
    
    fragColor = vec4(texColor); 
} 

