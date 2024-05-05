//--------------------------------------------------------------------------------------
// Order Independent Transparency with Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

uniform sampler2DRect DepthTex;

out vec4 FragColor;

vec4 ShadeFragment();

void main(void)
{
	// Bit-exact comparison between FP32 z-buffer and fragment depth
	float frontDepth = texture2DRect(DepthTex, gl_FragCoord.xy).r;
	if (gl_FragCoord.z <= frontDepth) {
		discard;
	}

	// Shade all the fragments behind the z-buffer
	vec4 color = ShadeFragment();
	FragColor = vec4(color.rgb * color.a, color.a);
}
