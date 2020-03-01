#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

 #extension GL_GOOGLE_include_directive : enable

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;

#include "includes/material_ids.glsl"
#include "includes/mrt_target_outputs.glsl"
#include "includes/push_constant_material.glsl"

void main() 
{
	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	N.y = -N.y;
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(N, 1.0);
	outAlbedo = vec4(material.baseColorFactor.rgb, material.specularFactor.r);
	outMaterial = MATERIAL_DEFAULT;
}