#version 330 core

layout(location = 0) in vec3 VertexPosition;

uniform mat4 ModelViewProjectionMatrix;

void ShadeVertex();

void main(void)
{
	gl_Position = ModelViewProjectionMatrix * vec4(VertexPosition, 1.);
	ShadeVertex();
}
