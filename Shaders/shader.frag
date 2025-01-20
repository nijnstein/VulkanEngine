#version 450

layout(binding = 2) uniform sampler2D texSampler;
layout(binding = 3) uniform sampler2D normalSampler; 

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal; 
layout(location = 3) in vec3 lightPosition; 
layout(location = 4) in vec3 fragPosition;
layout(location = 5) in vec3 viewPosition;
layout(location = 6) in vec3 tangent; 
layout(location = 7) in vec3 lightDirection; 

layout(location = 0) out vec4 outColor;


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

void main()
{
    float ambientStrength = 0.01;
    float specStrength = 1.7;
    float shininess = 32; 
    vec3 lightColor = vec3(1, 1, 0.9); 

    vec3 ambient = ambientStrength * lightColor;
    vec3 view_dir = normalize(viewPosition - fragPosition);
                
    vec3 normal = perturb_normal(normalize(fragNormal), view_dir, fragTexCoord); 
    
  
    float power = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = power * lightColor;
  
    vec3 reflect_dir = reflect(-lightDirection, normal);

    float spec = 0; 

    if( power <= 0.0 ) 
    {
        // discard the specular highlight if the light's behind the vertex
        spec = 0;
    }
    else
    {
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess) * specStrength;
    }
        
    vec3 result = (ambient + diffuse + 0.5 * spec) * fragColor + 0.5f * spec * lightColor;     

    outColor = texture(texSampler, fragTexCoord) * vec4(result, 1);
}