#version 330 core

layout (location = 0) in vec4 Vertex; // <vec2 pos, vec2 tex>

uniform mat4 Projection;

out vec2 TexCoords;

void main()
{
    gl_Position = Projection * vec4(Vertex.xy, 0.0, 1.0);
    TexCoords = Vertex.zw;
}
