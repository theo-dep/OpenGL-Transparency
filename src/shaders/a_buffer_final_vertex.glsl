#version 420 core

layout(location = 0) in vec3 VertexPosition;

void main(void)
{
    gl_Position = vec4(VertexPosition, 1.);
}
