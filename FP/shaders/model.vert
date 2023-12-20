#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec2 inTan;
layout(location = 4) in vec2 inBitan;

out VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
} vs_out;

out vec4 fragPosLightSpace;
out vec3 normal;
out vec2 texCoord;

uniform mat4 MM;
uniform mat4 MV;
uniform mat4 MP;

// Lighting
uniform mat4 MDSM;
uniform vec3 lightPos;

void main() {
    vec4 lightPosView = MV * vec4(lightPos, 1.0);
    vec4 P = MV * MM * vec4(inPos, 1.0);
    vs_out.N = mat3(MV * MM) * inNormal;
    vs_out.L = lightPosView.xyz - P.xyz;
    vs_out.V = - P.xyz;

    fragPosLightSpace = MDSM * vec4(inPos, 1.0);
    normal = inNormal;
    texCoord = inTex;
    gl_Position = MP * P;
}