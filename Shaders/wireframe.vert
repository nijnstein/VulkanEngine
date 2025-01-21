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


layout(location = 0) in uvec4 Q;

layout(location = 0) out vec3 fragColor;

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


void main()
{
    vec3 pos = positions.data[gl_InstanceIndex].xyz; 
    vec3 scale = scales.data[gl_InstanceIndex].xyz; 

    fragColor = getColor() * colors.data[gl_InstanceIndex].xyz;
    vec3 fragPosition = getPosition() * scale + pos;
    
    gl_Position = ubo.vp * vec4(fragPosition, 1.0);
}