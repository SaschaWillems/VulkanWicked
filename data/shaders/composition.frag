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
layout (binding = 3) uniform usampler2D samplerMaterial;
layout (binding = 4) uniform sampler2D samplerPBR;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (binding = 5) uniform UBO 
{
	Light lights[512];
	vec4 viewPos;
	int numLights;
	float fade;
	float desaturate;
} ubo;

#include "includes/material_ids.glsl"

vec3 calculateLight(Light light, vec3 pos, vec3 albedo)
{
	// Calculate light distance only on x/z plane
	vec2 L = light.position.xz - pos.xz;
	float dist = length(L);
	float atten = light.radius / (pow(dist, 2.0) + 1.0);
	return albedo.rgb * light.color * atten;
}

void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	uint material = texture(samplerMaterial, inUV).r;
	
	vec3 fragcolor  = vec3(0.0);
	vec3 diffuseColor = vec3(0.1);
	vec3 specColor = vec3(1.0);
 
	if (material == MATERIAL_DEFAULT || material == MATERIAL_SPORE)
	{
		for(int i = 0; i < ubo.numLights; ++i)
		{
			fragcolor += calculateLight(ubo.lights[i], fragPos, albedo.rgb);
		}
	}

//	if (material == MATERIAL_GLTF_PBR) 
//	{
//		fragcolor.rgb = pbrLighting();
//	}
//
	if (material == MATERIAL_TAROT_CARD)
	{
		fragcolor.rgb = albedo.rgb;
	}

    if (ubo.desaturate > 0.0) 
    {
	    vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), fragcolor.rgb));
	    outFragcolor = vec4(mix(fragcolor.rgb, gray, ubo.desaturate) * ubo.fade, 1.0);
    } 
    else 
    {
        outFragcolor = vec4(fragcolor.rgb * ubo.fade, 1.0);
    }
}