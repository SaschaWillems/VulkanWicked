#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

// Instanced attributes
layout (location = 3) in vec3 instancePos;
layout (location = 4) in float instanceScale;

layout (set = 0, binding = 0) uniform UBOCamera
{
	mat4 projection;
	mat4 view;
} camera;

layout (set = 1, binding = 0) uniform UBODummy
{
	mat4 model;
} dummy;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;

void main() 
{
	vec4 tmpPos = vec4(instancePos + (inPos * instanceScale) , 1.0);
	gl_Position = camera.projection * camera.view * tmpPos;
	outUV = inUV;
	outUV.t = 1.0 - outUV.t;
	outWorldPos = vec3(tmpPos);
	outWorldPos.y = -outWorldPos.y;
	outNormal = normalize(inNormal);	
	outTangent = inNormal * normalize(vec3(1.0f));
	outColor = vec3(1.0f);
}
