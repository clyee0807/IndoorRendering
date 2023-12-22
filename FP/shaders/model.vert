#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inTan;
layout(location = 4) in vec3 inBitan;

out VS_OUT {
    vec3 T;
    vec3 B;
    vec3 N_N;
    vec3 N_L;
    vec3 L;
    vec3 V;
} vs_out;

out vec4 fragPosLightSpace;
out vec3 normal;
out vec2 texCoords;

// MVP
uniform mat4 MM;
uniform mat4 MV;
uniform mat4 MP;

// Lighting
uniform mat4 MDSM;
uniform vec3 lightPos;



/* ---------------------------- MAIN ---------------------------- */
void main() {
    // Eye space to tangent space TBN
    vs_out.T  = mat3(MV * MM) * inTan;
    vs_out.B  = mat3(MV * MM) * inBitan;
    vs_out.N_N = inNormal;

    // N, L, V
    vec4 lightPosView = MV * vec4(lightPos, 1.0);
    vec4 P    = MV * MM * vec4(inPos, 1.0);
    vs_out.N_L = mat3(MV * MM) * inNormal;
    vs_out.L  = lightPosView.xyz - P.xyz;
    vs_out.V  = - P.xyz;

    fragPosLightSpace = MDSM * vec4(inPos, 1.0);
    texCoords = inTex;
    gl_Position = MP * P;
}