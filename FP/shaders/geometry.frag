#version 450 core

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outDiffuse;
layout(location = 3) out vec3 outAmbient;
layout(location = 4) out vec3 outSpecular;

in VS_OUT {
    vec3 fragPos;
    // out vec3 fragNormal;
    vec2 texCoords;
    mat3 TBN;
} fs_in;

// Shading
struct Material {
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
    float Ns;
};
uniform Material material;
uniform bool hasTex;
uniform bool hasNM;
uniform sampler2D tex;
uniform sampler2D normalMap;

// Options
uniform bool useNM;



/* ---------------------------- MAIN ---------------------------- */
void main() {
    vec4 texColor = texture(tex, fs_in.texCoords);
    vec3 normalMap = texture(normalMap, fs_in.texCoords).rgb;
    normalMap = normalMap * 2.0 - 1.0;

    outPos = fs_in.fragPos;
    outNormal = (hasNM && useNM) ? normalize(fs_in.TBN * normalMap) : fs_in.TBN[2];
    outDiffuse = hasTex ? texColor.rgb : material.Kd.rgb;
    // outAmbient = material.Ka.rgb;
    outAmbient = material.Kd.rgb;
    outSpecular = material.Ks.rgb;
}