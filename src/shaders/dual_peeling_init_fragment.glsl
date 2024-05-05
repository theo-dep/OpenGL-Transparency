//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

out vec4 FragColor;

void main(void)
{
	FragColor.xy = vec2(-gl_FragCoord.z, gl_FragCoord.z);
}
