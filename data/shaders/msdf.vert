#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (push_constant) uniform PushConstants {
	vec2 scale;
	vec2 translate;
} pushConstants;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;

void main() 
{
	outUV = inUV;
    outColor = inColor;
	gl_Position = vec4(inPos.xy * pushConstants.scale + pushConstants.translate, inPos.z, 1.0);
}