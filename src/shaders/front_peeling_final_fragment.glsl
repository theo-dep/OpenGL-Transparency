//--------------------------------------------------------------------------------------
// Order Independent Transparency with Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

uniform sampler2DRect ColorTex;
uniform vec3 BackgroundColor;

out vec4 FragColor;

void main(void)
{
	vec4 frontColor = texture2DRect(ColorTex, gl_FragCoord.xy);
	FragColor.rgb = frontColor.rgb + BackgroundColor * frontColor.a;
}
