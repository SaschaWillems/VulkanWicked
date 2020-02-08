// Get G-Buffer values
vec3 fragPos = texture(samplerposition, inUV).rgb;
vec3 normal = texture(samplerNormal, inUV).rgb;
vec4 albedo = texture(samplerAlbedo, inUV);
	
#define ambient 0.05	

vec3 fragcolor  = albedo.rgb * ambient;
if (albedo.a < 1.0f) 
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
else 
{
	fragcolor.rgb = albedo.rgb;
}