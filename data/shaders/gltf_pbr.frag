#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#extension GL_GOOGLE_include_directive : enable

layout (set = 2, binding = 0) uniform sampler2D colorMap;
layout (set = 2, binding = 1) uniform sampler2D physicalDescriptorMap;
layout (set = 2, binding = 2) uniform sampler2D normalMap;
layout (set = 2, binding = 3) uniform sampler2D aoMap;
layout (set = 2, binding = 4) uniform sampler2D emissiveMap;

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
	vec3 T = normalize(inTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(normalMap, inUV).xyz * 2.0 - vec3(1.0));
	outNormal.rgb = tnorm;
	outPosition = vec4(inWorldPos, 1.0);
	outAlbedo = texture(colorMap, inUV) * vec4(inColor, 1.0);
	outMaterial = MATERIAL_GLTF_PBR;
	outPBR = vec4(texture(physicalDescriptorMap, inUV).rgb, outAlbedo.a);
}