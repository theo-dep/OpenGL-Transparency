//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

//uniform sampler2DRect DepthBlenderTex;
uniform sampler2DRect FrontBlenderTex;
uniform sampler2DRect BackBlenderTex;

out vec4 FragColor;

void main(void)
{
	vec4 frontColor = texture2DRect(FrontBlenderTex, gl_FragCoord.xy);
	vec3 backColor = texture2DRect(BackBlenderTex, gl_FragCoord.xy).rgb;
	float alphaMultiplier = 1.0 - frontColor.w;

	// front + back
	FragColor.rgb = frontColor.rgb + backColor * alphaMultiplier;

	// front blender
	//FragColor.rgb = frontColor + vec3(alphaMultiplier);

	// back blender
	//FragColor.rgb = backColor;
}
