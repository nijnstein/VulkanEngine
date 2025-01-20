#version 450

layout(binding = 2) uniform sampler2D texSampler;
layout(binding = 3) uniform sampler2D normalSampler; 
layout(binding = 4) uniform sampler2D metallicSampler; 
layout(binding = 5) uniform sampler2D roughnessSampler; 

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal; 
layout(location = 3) in vec3 lightPosition; 
layout(location = 4) in vec3 fragPosition;
layout(location = 5) in vec3 viewPosition;
layout(location = 6) in vec3 tangent; 
layout(location = 7) in vec3 lightDirection; 

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv ) 
{ 
    // get edge vectors of the pixel triangle 
    vec3 dp1 = dFdx( p ); 
    vec3 dp2 = dFdy( p ); 
    vec2 duv1 = dFdx( uv ); 
    vec2 duv2 = dFdy( uv );   
    
    // solve the linear system 
    vec3 dp2perp = cross( dp2, N ); 
    vec3 dp1perp = cross( N, dp1 ); 
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) ); 
    return mat3( T * invmax, B * invmax, N ); 
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord ) 
{ 
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye) 
    vec3 map = normalize(2 * texture(normalSampler, texcoord).rgb - 1); 
    
    #ifdef WITH_NORMALMAP_UNSIGNED 
        map = map * 255./127. - 128./127.; 
    #endif 
    
    #ifdef WITH_NORMALMAP_2CHANNEL 
        map.z = sqrt( 1. - dot( map.xy, map.xy ) ); 
    #endif 
    
   // #ifdef WITH_NORMALMAP_GREEN_UP 
        map.y = -map.y; 
    //#endif 
    
    mat3 TBN = cotangent_frame( N, -V, texcoord ); 
    return normalize( TBN * map ); 
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic, vec3 materialColor)
{
	vec3 F0 = mix(vec3(0.04), materialColor, metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 lightColor, vec3 materialColor)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic, materialColor);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(tangent.xyz);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main()
{
    float ambientStrength = 0.02;

    float roughness = texture(roughnessSampler, fragTexCoord).r; 
    float metallic = texture(metallicSampler, fragTexCoord).r; 
    vec3 materialColor = texture(texSampler, fragTexCoord).rgb;     

    vec3 lightColor = vec3(1, 1, 0.9); 
    vec3 ambient = ambientStrength * lightColor * materialColor;
    vec3 view_dir = normalize(viewPosition - fragPosition);
    vec3 normal = perturb_normal(normalize(fragNormal), view_dir, fragTexCoord); 

    vec3 light = ambient + BRDF(lightDirection, view_dir, normal, metallic, roughness, lightColor, materialColor);
        
    // gamma correct
    light = pow(light, vec3(0.4545));

    outColor = vec4(light, 1);
}