#version 450

layout(location=0) in  vec4 inPos;
layout(location=1) in  vec4 inColor;
layout(location=2) in  vec2 inUV;
layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outUV;


void main()
{
    gl_Position = inPos;
    outColor = inColor;
    outUV    = inUV;
}
