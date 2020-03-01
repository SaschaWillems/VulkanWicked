#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#extension GL_GOOGLE_include_directive : enable

//layout (binding = 1) uniform sampler2D samplerColor;
//layout (binding = 2) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

#include "includes/push_constant_material.glsl"

void main() 
{
	outPosition = vec4(inWorldPos, 1.0);

	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	N.y = -N.y;
//	vec3 T = normalize(inTangent);
//	vec3 B = cross(N, T);
//	mat3 TBN = mat3(T, B, N);
//	vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
//	outNormal = vec4(tnorm, 1.0);
//
	outNormal = vec4(N, 1.0);
	outAlbedo = vec4(material.baseColorFactor.r, material.baseColorFactor.g, material.baseColorFactor.b, 1.0);
}