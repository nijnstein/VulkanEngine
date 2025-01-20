#version 450

struct EntityData 
{
    vec3 position;
    vec3 rotation;
    vec3 scale; 
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 vp; 

    mat4 normal;
    vec3 lightPosition;
    vec3 viewPosition;
        
	vec3 lightDirection;
	vec3 lightColor; 
	float lightIntensity; 
} ubo;

layout(binding = 1) readonly buffer EntityDataArray {
	EntityData data[1024 * 1024];  
} entities;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal; 
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent; 
layout(location = 5) in vec3 inBiTangent; 

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 lightPosition; 
layout(location = 4) out vec3 fragPosition; 
layout(location = 5) out vec3 viewPosition; 
layout(location = 6) out vec3 tangent;
layout(location = 7) out vec3 lightDirection;



void main()
{         
    EntityData data = entities.data[gl_InstanceIndex]; 

    fragColor = inColor;
    fragTexCoord = inTexCoord;



    fragPosition = inPosition * data.scale + data.position;
  
    fragNormal = inNormal;    //normalize(mat3(ubo.normal) * inNormal);  
    
    gl_Position = ubo.vp * vec4(fragPosition, 1.0);

    lightPosition = ubo.lightPosition;
    viewPosition = ubo.viewPosition;
    lightDirection = ubo.lightDirection; 

    tangent = inTangent;     
}