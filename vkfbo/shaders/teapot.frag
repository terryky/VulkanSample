#version 450

layout(location=0) in  vec3 inDiffuse;
layout(location=1) in  vec3 inSpecular;
layout(location=0) out vec4 outColor;

layout(binding=1) uniform fs_ubo
{
    vec4 u_color;
};

void main()
{
    vec3 color;
    color = vec3 (0.1, 0.1, 0.1);
    color += vec3 (u_color) * inDiffuse;
    color += inSpecular;

    outColor = vec4(color, 1.0);
}
