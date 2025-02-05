#version 450

layout(location = 0) in float near; //0.01
layout(location = 1) in float far;  //100

layout(location = 2) in vec3 nearPoint;
layout(location = 3) in vec3 farPoint;
layout(location = 4) in mat4 view;
layout(location = 8) in mat4 projection;
layout(location = 0) out vec4 outColor;

vec4 grid(vec3 pos, float scale, bool drawAxis)
{
    vec2 coord = pos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);

    vec4 color = vec4(0.05, 0.05, 0.05, 1.0 - min(line, 1.0));

    if(pos.x > -0.1 * minimumx && pos.x < 0.1 * minimumx)
        color.z = 1.0;
    
    if(pos.z > -0.1 * minimumz && pos.z < 0.1 * minimumz)
        color.x = 1.0;

    return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far; // normalize
}

void main()
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 pos = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(pos);

    float linearDepth = computeLinearDepth(pos);
    float fading = max(0, (0.5 - linearDepth));

    // Display only the lower plane
    outColor = (grid(pos, 10, true) + grid(pos, 1, true)) * float(t > 0); // adding multiple resolution for the grid
    outColor.a *= fading;
}