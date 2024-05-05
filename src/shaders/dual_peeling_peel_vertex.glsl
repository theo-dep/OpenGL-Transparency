//--------------------------------------------------------------------------------------
// Order Independent Transparency with Dual Depth Peeling
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

layout(location = 0) in vec3 VertexPosition;

uniform mat4 ModelViewProjectionMatrix;

void ShadeVertex();

void main(void)
{
	gl_Position = ModelViewProjectionMatrix * vec4(VertexPosition, 1.);
	ShadeVertex();
}
