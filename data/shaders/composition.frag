#version 450

/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#extension GL_GOOGLE_include_directive : enable

#include "includes/material_ids.glsl"
#include "includes/deferred_lighting_inputs.glsl"

void main() 
{
    #include "includes/deferred_lighting.glsl"
	// Desaturate
	vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), fragcolor.rgb));
	outFragcolor.rgb = mix(fragcolor.rgb, gray, 0.5f);  
}