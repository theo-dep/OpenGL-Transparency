#version 330 core

out vec4 FragColor;

vec4 ShadeFragment();

void main(void)
{
	FragColor = ShadeFragment();
}
