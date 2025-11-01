#version 450 core

layout (location = 0) in vec3 vertexColor;
layout (location = 1) in vec2 texCoord;
layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D ourTexture;

void main()
{
   vec4 texColor = texture(ourTexture, texCoord);
    
    // Debug: If texture is all zeros (black), show red
    if (texColor.r == 0.0 && texColor.g == 0.0 && texColor.b == 0.0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red = texture returned black
    } else {
        FragColor = vec4(texColor.rgb, 1.0); // Normal texture color
    }
} 