//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Alec
//uniform samplerRECT DepthBlenderTex;
//uniform samplerRECT FrontBlenderTex;
//uniform samplerRECT BackBlenderTex;
uniform sampler2DRect DepthBlenderTex;
uniform sampler2DRect FrontBlenderTex;
uniform sampler2DRect BackBlenderTex;

void main(void)
{
        // Alec
	//vec4 frontColor = textureRect(FrontBlenderTex, gl_FragCoord.xy);
	//vec3 backColor = textureRect(BackBlenderTex, gl_FragCoord.xy).rgb;
	vec4 frontColor = texture2DRect(FrontBlenderTex, gl_FragCoord.xy);
	vec3 backColor = texture2DRect(BackBlenderTex, gl_FragCoord.xy).rgb;
	float alphaMultiplier = 1.0 - frontColor.w;

	// front + back
        // Alec
	//gl_FragColor.rgb = frontColor + backColor * alphaMultiplier;
	gl_FragColor.rgb = frontColor.rgb + backColor * alphaMultiplier;
	
	// front blender
	//gl_FragColor.rgb = frontColor + vec3(alphaMultiplier);
	
	// back blender
	//gl_FragColor.rgb = backColor;
}
