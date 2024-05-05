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

uniform float Alpha;

#define COLOR_FREQ 30.0
#define ALPHA_FREQ 30.0

vec4 ShadeFragment()
{
#if 1
	float xWorldPos = TexCoord.x;
	float yWorldPos = TexCoord.y;
	float diffuse = TexCoord.z;

	vec4 color;
	float i = floor(xWorldPos * COLOR_FREQ);
	float j = floor(yWorldPos * ALPHA_FREQ);
	color.rgb = (mod(i, 2.0) == 0.0) ? vec3(.4,.85,.0) : vec3(1.0);
	//color.a = (fmod(j, 2.0) == 0) ? Alpha : 0.2;
	color.a = Alpha;

	color.rgb *= diffuse;
	return color;
#else
	vec4 color;
	color.rgb = vec3(.4,.85,.0);
	color.a = Alpha;
	return color;
#endif
}
