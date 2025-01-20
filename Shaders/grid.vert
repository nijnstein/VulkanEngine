#version 450

layout(binding = 0) uniform Ubo {
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 viewProjectionInverse;
} ubo;


layout(location = 0) out float near; //0.01
layout(location = 1) out float far;  //100
layout(location = 2) out vec3 nearPoint;
layout(location = 3) out vec3 farPoint;
layout(location = 4) out mat4 view;
layout(location = 8) out mat4 projection;


vec3 unprojectPoint(float x, float y, float z, mat4 viewProjectionInverse) {
    vec4 clipSpacePos = vec4(x, y, z, 1.0);
    vec4 eyeSpacePos = viewProjectionInverse * clipSpacePos;
    return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

void main()
{
    vec3 pos = gridPlane[gl_VertexIndex].xyz;

    near = 0.01f;
    far = 100.0f;

    nearPoint = unprojectPoint(pos.x, pos.y, 0.0, ubo.viewProjectionInverse);
    farPoint = unprojectPoint(pos.x, pos.y, 1.0, ubo.viewProjectionInverse);

    view = ubo.view;
    projection = ubo.projection;

    gl_Position = vec4(pos, 1.0);
}