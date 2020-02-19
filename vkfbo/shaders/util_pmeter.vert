#version 450

layout(location=0) in  vec4 i_Vertex;
layout(location=0) out vec4 o_color;

layout(binding=0) uniform Matrices
{
    vec4 u_PrjMul;
    vec4 u_PrjAdd;
};

struct InstanceData
{
    vec4 ofst_vtx;
    vec4 color;
};

layout(binding=1) uniform InstanceParameters
{
    InstanceData instance_data[128];
};

void main()
{
    vec4 pos = i_Vertex;
    pos = pos + instance_data[gl_InstanceIndex].ofst_vtx;
    pos = pos * u_PrjMul;
    pos = pos + u_PrjAdd;

    gl_Position = pos;

    o_color = instance_data[gl_InstanceIndex].color;
}
