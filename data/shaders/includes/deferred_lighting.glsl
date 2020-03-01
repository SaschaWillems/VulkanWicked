// Get G-Buffer values
vec3 fragPos = texture(samplerposition, inUV).rgb;
vec3 normal = texture(samplerNormal, inUV).rgb;
vec4 albedo = texture(samplerAlbedo, inUV);
uint material = texture(samplerMaterial, inUV).r;
	
const float ambient = albedo.a;

vec3 fragcolor  = albedo.rgb * ambient;

if (material == MATERIAL_DEFAULT || material == MATERIAL_SPORE)
{
	for(int i = 0; i < ubo.numLights; ++i)
	{
		vec3 L = ubo.lights[i].position.xyz - fragPos;
		float dist = length(L);
		vec3 V = ubo.viewPos.xyz - fragPos;
		V = normalize(V);	
		float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);
		vec3 lightDir = normalize(L);
		vec3 diffuse = max(dot(normalize(normal), lightDir), 0.0f) * albedo.rgb * ubo.lights[i].color * atten;
		fragcolor += diffuse;
	}
}

if (material == MATERIAL_TAROT_CARD)
{
	fragcolor.rgb = albedo.rgb;
}