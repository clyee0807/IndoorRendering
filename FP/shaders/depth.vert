#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inTan;
layout(location = 4) in vec3 inBitan;

//uniform mat4 lightSpaceMatrix;
uniform mat4 MDM;

void main()
{
    gl_Position = MDM * vec4(inPos, 1.0f);
}