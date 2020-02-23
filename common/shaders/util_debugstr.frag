#version 450

layout(location=0) in  vec2 i_UV;
layout(location=1) in  vec4 i_col_fg;
layout(location=2) in  vec4 i_col_bg;
layout(location=0) out vec4 o_Color;

layout(binding=5) uniform sampler2D u_Sampler0;

void main()
{
    vec4 texcol = texture(u_Sampler0, i_UV);
    o_Color = mix (i_col_bg, i_col_fg, texcol.r);
}
