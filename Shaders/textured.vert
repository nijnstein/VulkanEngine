#version 450

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

layout(binding = 1) readonly buffer EntityPositionArray {
	vec4 data[1024 * 1024];  
} positions;

layout(binding = 2) readonly buffer EntityRotationArray {
	vec4 data[1024 * 1024];  
} rotations;

layout(binding = 3) readonly buffer EntityScaleArray {
	vec4 data[1024 * 1024];  
} scales;

layout(binding = 4) readonly buffer EntityColorArray {
	vec4 data[1024 * 1024];  
} colors;

layout(binding = 5) readonly buffer EntityIndexArray {
	int data[1024 * 1024];  
} entities;


layout(location = 0) in uvec4 Q;



layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 lightPosition; 
layout(location = 4) out vec3 fragPosition; 
layout(location = 5) out vec3 viewPosition; 
layout(location = 6) out vec3 tangent;
layout(location = 7) out vec3 lightDirection;

vec3 getPosition()
{
    float x = (Q.x & 0x0000FFFF) ;
    float y = ((Q.x & 0xFFFF0000) >> 16) ; 
    float z = (Q.y & 0x0000FFFF);
    return vec3(x / 64.0f - 512.0f, y / 64.0f - 512.0f, z / 64.0f - 512.0f);
}

vec3 getColor()
{
    float x = (Q.y & 0x00FF0000) >> 16;
    float y = (Q.y & 0xFF000000) >> 24;
    float z = (Q.z & 0x000000FF);
    return vec3(x / 255.0f, y / 255.0f, z / 255.0f);
}

vec3 getNormal()
{
    float x = (Q.z & 0x0000FF00) >> 8;
    float y = (Q.z & 0x00FF0000) >> 16;
    float z = (Q.z & 0xFF000000) >> 24;
    return vec3((x / 127.0f) - 1.0f, (y / 127.0f) - 1.0f, (z / 127.0f) - 1.0f);
}

vec2 getUV()
{
    float u = (Q.w & 0x0000FFFF) / 255.0f;
    float v = ((Q.w & 0xFFFF0000) >> 16) / 255.0f;
    return vec2(u, v); 
}

void main()
{
    int entityId = entities.data[gl_InstanceIndex]; 

    vec3 pos = positions.data[entityId].xyz; 
    vec3 scale = scales.data[entityId].xyz; 

    fragColor = getColor();
    fragTexCoord = getUV();
    fragPosition = getPosition() * scale + pos;

    fragNormal = normalize(mat3(ubo.normal) * getNormal());  
    lightDirection = ubo.lightDirection;  
    
    gl_Position = ubo.vp * vec4(fragPosition, 1.0);

    lightPosition = ubo.lightPosition;
    viewPosition = ubo.viewPosition; 
}