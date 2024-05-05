//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

uniform sampler2DRect DepthBlenderTex;
uniform sampler2DRect FrontBlenderTex;

layout(location = 0) out vec2 Depth;
layout(location = 1) out vec4 FrontColor;
layout(location = 2) out vec4 BackColor;

#define MAX_DEPTH 1.0

vec4 ShadeFragment();

void main(void)
{
	// window-space depth interpolated linearly in screen space
	float fragDepth = gl_FragCoord.z;

	vec2 depthBlender = texture2DRect(DepthBlenderTex, gl_FragCoord.xy).xy;
	vec4 forwardTemp = texture2DRect(FrontBlenderTex, gl_FragCoord.xy);

	// Depths and 1.0-alphaMult always increase
	// so we can use pass-through by default with MAX blending
	Depth = depthBlender;

	// Front colors always increase (DST += SRC*ALPHA_MULT)
	// so we can use pass-through by default with MAX blending
	FrontColor = forwardTemp;

	// Because over blending makes color increase or decrease,
	// we cannot pass-through by default.
	// Each pass, only one fragment writes a color greater than 0
	BackColor = vec4(0.0);

	float nearestDepth = -depthBlender.x;
	float farthestDepth = depthBlender.y;
	float alphaMultiplier = 1.0 - forwardTemp.w;

	if (fragDepth < nearestDepth || fragDepth > farthestDepth) {
		// Skip this depth in the peeling algorithm
		Depth = vec2(-MAX_DEPTH);
		return;
	}

	if (fragDepth > nearestDepth && fragDepth < farthestDepth) {
		// This fragment needs to be peeled again
		Depth = vec2(-fragDepth, fragDepth);
		return;
	}

	// If we made it here, this fragment is on the peeled layer from last pass
	// therefore, we need to shade it, and make sure it is not peeled any farther
	vec4 color = ShadeFragment();
	Depth = vec2(-MAX_DEPTH);

	if (fragDepth == nearestDepth) {
		FrontColor.xyz += color.rgb * color.a * alphaMultiplier;
		FrontColor.w = 1.0 - alphaMultiplier * (1.0 - color.a);
	} else {
		BackColor += color;
	}
}
