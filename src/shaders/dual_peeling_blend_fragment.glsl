//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Alec
//uniform samplerRECT TempTex;
uniform sampler2DRect TempTex;

void main(void)
{
        // Alec
	//gl_FragColor = textureRect(TempTex, gl_FragCoord.xy);
	gl_FragColor = texture2DRect(TempTex, gl_FragCoord.xy);
	// for occlusion query
        // Alec
	//if (gl_FragColor.a == 0) discard;
	if (gl_FragColor.a == 0.0) discard;
}
