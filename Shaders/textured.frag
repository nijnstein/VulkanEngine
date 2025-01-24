#version 450

layout(binding = 6) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal; 
layout(location = 3) in vec3 lightPosition; 
layout(location = 4) in vec3 fragPosition;
layout(location = 5) in vec3 viewPosition;
layout(location = 6) in vec3 tangent; 
layout(location = 7) in vec3 lightDirection; 

layout(location = 0) out vec4 outColor;

void main()
{
    float ambientStrength = 0.1;
    float specStrength = 1.7;
    float shininess = 32; 
    vec3 lightColor = vec3(1, 1, 0.9); 

    vec3 ambient = ambientStrength * lightColor;
    vec3 view_dir = normalize(viewPosition - fragPosition);
                
    vec3 normal = fragNormal;     
    float power = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = power * lightColor;
  
    vec3 reflect_dir = reflect(-lightDirection, normal);

    float spec = 0; 

    if( power <= 0.0 ) 
    {
        spec = 0;
    }
    else
    {
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess) * specStrength;
    }
 

    vec3 tex = texture(texSampler, fragTexCoord).rgb; 

    vec3 result = (ambient + diffuse + spec) * tex;     

    outColor = vec4(result, 1.0);
}