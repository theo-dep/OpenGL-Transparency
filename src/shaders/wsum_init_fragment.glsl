//--------------------------------------------------------------------------------------
// Order Independent Transparency with Weighted Sums
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

vec4 ShadeFragment();

out vec4 FragColor;

void main(void)
{
	vec4 color = ShadeFragment();
	FragColor = vec4(color.rgb * color.a, color.a);
}
