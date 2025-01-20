#version 450

struct EntityTransform 
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
} ubo;

layout(binding = 1) readonly buffer EntityDataArray {
	EntityTransform data[1024 * 1024];  
} entities;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition; 

void main()
{
    EntityData data = entities.data[gl_InstanceIndex]; 

    fragColor = inColor * data.color;
    fragPosition = inPosition * data.scale + data.position;
    gl_Position = ubo.vp * vec4(fragPosition, 1.0);
}