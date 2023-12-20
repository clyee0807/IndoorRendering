#version 450 core

layout(location = 0) out vec4 fragColor;
//layout (location = 0) out vec4 gNormal;
//layout (location = 1) out vec3 gAlbedo;
//layout (location = 2) out vec4 gSpecular;

in VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
} fs_in;

in vec3 posForColoring;
in vec2 texCoord;

uniform int useNormalMap;
uniform bool hasTex;
uniform bool hasNormalMap;
uniform sampler2D tex;
uniform sampler2D normalMap;

// Blinn-Phong
struct Material {
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
    float Ns;
};
uniform Material material;
uniform vec3 Ia = vec3(0.1);
uniform vec3 Id = vec3(0.7);
uniform vec3 Is = vec3(0.2);

void main() {
    vec4 textureColor = texture(tex, texCoord);
    vec3 normalMap = texture(normalMap, texCoord).rgb;
    normalMap = normalMap * 2.0 - 1.0;

    // Blinn-Phong: Vectors
    vec3 N;
    if (hasNormalMap && useNormalMap != 0) {
        N = normalize(normalMap);
    } else {
        N = normalize(fs_in.N);
    }
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);

    // Blinn-Phong: Ambient, Diffuse, Specular
    vec3 Ka, Kd, Ks;
    if (hasTex) {
        Kd = textureColor.rgb;
    } else {
        Kd = material.Kd.rgb;
    }
    vec3 ambient  = Ia * material.Ka.rgb;
    vec3 diffuse  = Id * Kd * max(dot(N, L), 0.0);
    vec3 specular = Is * material.Ks.rgb * pow(max(dot(N, H), 0.0), material.Ns);

    // Output
    if (textureColor.a < 0.5) {
        discard;
    } else {
        fragColor = vec4(ambient + diffuse + specular, 1.0);
    }
    /*
    gNormal.xyz = N;
    gNormal.w = posForColoring.z;
    gAlbedo = Kd;
    gSpecular.rgb = specular;
    gSpecular.a = 20.0f;
    */
}