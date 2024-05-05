//--------------------------------------------------------------------------------------
// Order Independent Transparency with Average Color
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

layout(location = 0) out vec4 Color0;
layout(location = 1) out vec4 Color1;

vec4 ShadeFragment();

void main(void)
{
	vec4 color = ShadeFragment();
	Color0 = vec4(color.rgb * color.a, color.a);
	Color1 = vec4(1.0);
}
