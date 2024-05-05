//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

uniform sampler2DRect TempTex;

out vec4 FragColor;

void main(void)
{
	FragColor = texture2DRect(TempTex, gl_FragCoord.xy);
	// for occlusion query
	if (FragColor.a == 0.0) discard;
}
