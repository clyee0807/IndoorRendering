#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inTan;
layout(location = 4) in vec3 inBitan;

out VS_OUT {
    vec3 fragPos;
    // out vec3 fragNormal;
    vec2 texCoords;
    mat3 TBN;
} vs_out;

// MVP
uniform mat4 MM;
uniform mat4 MV;
uniform mat4 MP;



/* ---------------------------- MAIN ---------------------------- */
void main() {
    vec4 worldPos = MM * vec4(inPos, 1.0);

    // Eye space to tangent space TBN
    vec3 T = normalize(mat3(MV * MM) * inTan);
    vec3 B = normalize(mat3(MV * MM) * inBitan);
    vec3 N = normalize(inNormal);
    vs_out.TBN = mat3(T, B, N);

    gl_Position = MP * MV * worldPos;
    vs_out.fragPos = worldPos.xyz;
    // fragNormal = N;
    vs_out.texCoords = inTex;
}