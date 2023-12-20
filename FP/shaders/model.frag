#version 450 core

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
} fs_in;

in vec4 fragPosLightSpace;
in vec3 normal;
in vec2 texCoord;

uniform int useNormalMap;
uniform bool hasTex;
uniform bool hasNormalMap;
uniform sampler2D tex;
uniform sampler2D normalMap;

// Lighting
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
// Directional Shadow Mapping
uniform int sampleRadius;
uniform float shadowBias;
uniform float biasVariation;
uniform sampler2D shadowMap;

float dsmCalculation(vec3 N, vec3 L) {
    vec3 lightCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    float shadow = 0.0f;

    if (lightCoords.z <= 1.0f) {
        lightCoords = lightCoords * 0.5 + 0.5;

        float currentDepth = lightCoords.z;
        float bias = max(biasVariation * (1.0 - dot(N, L)), shadowBias);
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

        for (int y = -sampleRadius; y <= sampleRadius; y++) {
            for (int x = -sampleRadius; x <= sampleRadius; x++) {
                float closestDepth = texture(shadowMap, lightCoords.xy + vec2(x, y) * texelSize).r;
                if (currentDepth > closestDepth + bias) {
                    shadow += 1.0f;
                }
            }
        }
        shadow /= pow((sampleRadius * 2 + 1), 2);
    }
    return shadow;
}

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

    // Directional Shadow Mapping
    float shadow = dsmCalculation(N, L);

    // Output
    if (textureColor.a < 0.5) {
        discard;
    } else {
        fragColor = vec4(ambient + (1.0f - shadow) * diffuse + specular, 1.0);
    }
}