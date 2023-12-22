#version 450 core

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec3 T;
    vec3 B;
    vec3 N_N;
    vec3 N_L;
    vec3 L;
    vec3 V;
} fs_in;

in vec4 fragPosLightSpace;
in vec2 texCoord;

// Shading
uniform bool hasTex;
uniform bool hasNM;
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
struct LightingComponents {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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

// Options
uniform bool useNM;
uniform bool enableBP;
uniform bool enableDSM;
uniform bool enableCEL;



/* -------------------------- LIGHTING -------------------------- */
LightingComponents bpCalculation(vec3 N, vec3 L, vec3 V, vec3 Kd, Material material) {
    LightingComponents components;
    vec3 H = normalize(L + V);

    components.ambient  = Ia * material.Ka.rgb;
    components.diffuse  = Id * Kd * max(dot(N, L), 0.0);
    components.specular = Is * material.Ks.rgb * pow(max(dot(N, H), 0.0), material.Ns);
    return components;
}

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



/* ---------------------------- MAIN ---------------------------- */
void main() {
    vec4 textureColor = texture(tex, texCoord);
    vec3 normalMap = texture(normalMap, texCoord).rgb;
    normalMap = normalMap * 2.0 - 1.0;

    // TBN
    vec3 T   = normalize(fs_in.T);
    vec3 B   = normalize(fs_in.B);
    vec3 NN  = normalize(fs_in.N_N);
    mat3 TBN = mat3(T, B, NN);

    // Lighting Vectors
    vec3 NL = (hasNM && useNM) ? normalize(TBN * normalMap) : normalize(fs_in.N_L);
    vec3 L  = normalize(fs_in.L);
    vec3 V  = normalize(fs_in.V);

    // Blinn-Phong
    vec3 Kd = hasTex ? textureColor.rgb : material.Kd.rgb;
    LightingComponents blinnPhong = bpCalculation(NL, L, V, Kd, material);

    // Directional Shadow Mapping
    float shadow = 0.0f;
    if (enableDSM) shadow = dsmCalculation(NL, L);

    // Output
    float diffuseFactor = (1.0f - shadow);
    vec3 result = enableBP
        ? (blinnPhong.ambient + blinnPhong.diffuse * diffuseFactor + blinnPhong.specular)
        : Kd * diffuseFactor;

    // Cel Shading
     if (enableCEL) {
        float intensity = dot(normalize(NL), normalize(L)); 
        if (intensity > 0.66) {
            result *= 1.25;  
        } else if (intensity > 0.33) {
            result *= 1.0;  
        } else {
            result *= 0.75;  
        }
    }

    if (textureColor.a < 0.5)
        discard;
    else
        fragColor = vec4(result, 1.0);
}