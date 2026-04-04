#version 450

// Fullscreen triangle from vertex index — no vertex buffer needed.
// Vertex 0: (-1, -1), Vertex 1: (3, -1), Vertex 2: (-1, 3)
// This covers the entire clip-space quad with a single triangle.

layout (location = 0) out vec2 outTexCoord;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 texCoords[3] = vec2[](
        vec2(0.0, 1.0),
        vec2(2.0, 1.0),
        vec2(0.0, -1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outTexCoord = texCoords[gl_VertexIndex];
}
