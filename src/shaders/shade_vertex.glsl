//--------------------------------------------------------------------------------------
// Order Independent Transparency Vertex Shader
//
// Author: Louis Bavoil
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#version 330 core

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;

out vec3 TexCoord;

void ShadeVertex()
{
    vec3 worldPosition = normalize((ModelViewMatrix * vec4(VertexPosition, 1)).xyz);
	vec3 normal = normalize(NormalMatrix * VertexNormal);

    const vec3 lightPosition = vec3(0, 0, 1);
    vec3 lightDir = normalize(lightPosition - worldPosition);

    float diffuse = abs(dot(normal, lightDir));
    TexCoord = vec3(VertexPosition.xy, diffuse);
}
