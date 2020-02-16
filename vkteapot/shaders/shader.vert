#version 450

layout(location=0) in  vec4 inPos;
layout(location=1) in  vec3 inNormal;
layout(location=2) in  vec2 inUV;
layout(location=0) out vec3 outDiffuse;
layout(location=1) out vec3 outSpecular;
layout(location=2) out vec2 outUV;

layout(binding=0) uniform Matrices
{
    mat4 u_PMVMatrix;
    mat4 u_MVMatrix;
    mat3 u_ModelViewIT;
};

const float shiness      = 16.0;
const vec3  LightPos     = vec3(4.0, 4.0, 4.0);
const vec3  LightCol     = vec3(1.0, 1.0, 1.0);


void DirectionalLight (vec3 normal, vec3 eyePos)
{
    vec3  lightDir = normalize (LightPos);
    vec3  halfV    = normalize (LightPos - eyePos);
    float dVP      = max(dot(normal, lightDir), 0.0);
    float dHV      = max(dot(normal, halfV   ), 0.0);

    float pf = 0.0;
    if(dVP > 0.0)
        pf = pow(dHV, shiness);

    outDiffuse += dVP * LightCol;
    outSpecular+= pf  * LightCol;
}


void main()
{
    gl_Position = u_PMVMatrix * inPos;
    vec3 normal = normalize(u_ModelViewIT * inNormal);
    vec3 eyePos = vec3(u_MVMatrix * inPos);

    outDiffuse  = vec3(0.0);
    outSpecular = vec3(0.0);
    DirectionalLight(normal, eyePos);

    outUV    = inUV;
}
