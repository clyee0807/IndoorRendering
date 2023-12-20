#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inTan;
layout(location = 4) in vec3 inBitan;

out VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
} vs_out;

out vec3 normal;
out vec2 texCoord;
out vec3 lightDir;
out vec3 eyeDir;

uniform mat4 MMV;
uniform mat4 MV;
uniform mat4 MP;
uniform vec3 position;
uniform vec3 lightPos = vec3(-2.845, 2.028, -1.293);

void main() {
    // N, L, V
    vec4 P = MMV * vec4(inPos, 1.0);
    vs_out.N = mat3(MMV) * inNormal;
    vec4 lightPosView = MV * vec4(lightPos, 1.0);
    vs_out.L = lightPosView.xyz - P.xyz;
    vs_out.V = - P.xyz;

    // Eye space to tangent space TBN
    vec3 T = normalize(mat3(MMV) * inTan);
    vec3 B = normalize(mat3(MMV) * inBitan);
    vec3 N = normalize(inNormal);
    mat3 TBN = mat3(T, B, N);





    texCoord = inTex;
    gl_Position = MP * P;
}