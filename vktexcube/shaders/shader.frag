#version 450

layout(location=0) in  vec3 inDiffuse;
layout(location=1) in  vec3 inSpecular;
layout(location=2) in  vec2 inUV;
layout(location=0) out vec4 outColor;

layout(binding=1) uniform sampler2D diffuseMap;

void main()
{
    vec3 color;
    color = vec3 (0.0, 0.0, 0.0);
    color += inDiffuse * vec3(texture(diffuseMap, inUV));
    color += inSpecular;

    outColor = vec4(color, 1.0);
}
