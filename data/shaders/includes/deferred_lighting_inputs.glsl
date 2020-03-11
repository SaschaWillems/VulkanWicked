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
} ubo;