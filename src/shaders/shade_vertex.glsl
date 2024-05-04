//--------------------------------------------------------------------------------------
// Order Independent Transparency Vertex Shader
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

void ShadeVertex()
{
	float diffuse = abs(normalize(gl_NormalMatrix * gl_Normal).z);
	gl_TexCoord[0].xyz = vec3(gl_Vertex.xy, diffuse);
}
