//--------------------------------------------------------------------------------------
// Order Independent Transparency with Weighted Sums
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Alec
uniform sampler2DRect ColorTex;
//uniform samplerRECT ColorTex;
uniform vec3 BackgroundColor;

// Sum(A_i * C_i) + C_bg * (1 - Sum(A_i))
void main(void)
{
  // Alec
	//vec4 S = textureRect(ColorTex, gl_FragCoord.xy);
	vec4 S = texture2DRect(ColorTex, gl_FragCoord.xy);
	gl_FragColor.rgb = S.rgb + BackgroundColor * (1.0 - S.a);
}
