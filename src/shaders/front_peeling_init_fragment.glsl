//--------------------------------------------------------------------------------------
// Order Independent Transparency with Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

out vec4 FragColor;

vec4 ShadeFragment();

void main(void)
{
	vec4 color = ShadeFragment();
	FragColor = vec4(color.rgb * color.a, 1.0 - color.a);
}
