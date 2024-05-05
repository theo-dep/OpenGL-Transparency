#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D Text0;
uniform vec3 TextColor;

void main()
{
    vec4 sampled = vec4(vec3(1.0), texture(Text0, TexCoords).r);
    FragColor = vec4(TextColor, 1.0) * sampled;
}
