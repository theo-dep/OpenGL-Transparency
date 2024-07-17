//--------------------------------------------------------------------------------------
// Order Independent Transparency Fragment Shader
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

in vec3 TexCoord;

uniform float Alpha = 1.0;

#define COLOR_FREQ 30.0
#define ALPHA_FREQ 30.0

vec4 ShadeFragment()
{
	float xWorldPos = TexCoord.x;
	float yWorldPos = TexCoord.y;
	float diffuse = TexCoord.z;

	vec4 color;
	float i = floor(xWorldPos * COLOR_FREQ);
	float j = floor(yWorldPos * ALPHA_FREQ);
	color.rgb = (mod(i, 2.0) == 0.0) ? vec3(.4, .85, .0) : vec3(1.0);
	color.a = Alpha;

	color.rgb *= diffuse;
	return color;
}
