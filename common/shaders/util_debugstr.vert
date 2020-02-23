#version 450

layout(location=0) in  vec4 i_Vertex;
layout(location=1) in  vec2 i_UV;
layout(location=0) out vec2 o_UV;
layout(location=1) out vec4 o_col_fg;
layout(location=2) out vec4 o_col_bg;

layout(binding=0) uniform Matrices
{
    vec4 u_PrjMul;
    vec4 u_PrjAdd;
};

struct InstanceData
{
    vec4 draw_dim;
    vec4 ofst_vtx;
    vec4 ofst_uv;
    vec4 col_fg;
    vec4 col_bg;
};

layout(binding=1) uniform InstanceParameters
{
    InstanceData instance_data[128];
};


void main()
{
    vec4 pos = i_Vertex;
    pos = pos * instance_data[gl_InstanceIndex].draw_dim;
    pos = pos + instance_data[gl_InstanceIndex].ofst_vtx;
    pos = pos * u_PrjMul;
    pos = pos + u_PrjAdd;

    gl_Position = pos;

    o_UV = i_UV + vec2(instance_data[gl_InstanceIndex].ofst_uv);
    o_col_fg = instance_data[gl_InstanceIndex].col_fg;
    o_col_bg = instance_data[gl_InstanceIndex].col_bg;
}
