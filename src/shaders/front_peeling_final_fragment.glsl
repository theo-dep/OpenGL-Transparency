//--------------------------------------------------------------------------------------
// Order Independent Transparency with Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Alec
//uniform samplerRECT ColorTex;
uniform sampler2DRect ColorTex;
uniform vec3 BackgroundColor;

void main(void)
{
  // Alec
	//vec4 frontColor = textureRect(ColorTex, gl_FragCoord.xy);
	vec4 frontColor = texture2DRect(ColorTex, gl_FragCoord.xy);
        // Alec
	//gl_FragColor.rgb = frontColor + BackgroundColor * frontColor.a;
	gl_FragColor.rgb = frontColor.rgb + BackgroundColor * frontColor.a;
}
