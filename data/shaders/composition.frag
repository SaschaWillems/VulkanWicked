#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#extension GL_GOOGLE_include_directive : enable

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (binding = 3) uniform UBO 
{
	Light lights[512];
	vec4 viewPos;
	int numLights;
} ubo;


void main() 
{
    #include "includes/deferred_lighting.glsl"
	// Desaturate
	vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), fragcolor.rgb));
	outFragcolor.rgb = mix(fragcolor.rgb, gray, 0.5f);  
}