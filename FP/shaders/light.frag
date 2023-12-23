#version 450 core

layout(location = 0) out vec4 fragColor;

in vec2 texCoords;

// MVP
uniform mat4 MV;
uniform mat4 MP;

// Lighting
uniform mat4 MDSM;
uniform vec3 lightPos;

// Shading
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gAmbient;
uniform sampler2D gSpecular;

// Options
uniform int displayType;
uniform bool enableBP;
uniform bool enableDSM;
uniform bool enableNPR;

uniform bool enablePL;
uniform bool enableAreaLight;
uniform bool enableVolumeLight;



// Blinn-Phong
struct LightingComponents {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform vec3 Ia = vec3(0.1);
uniform vec3 Id = vec3(0.7);
uniform vec3 Is = vec3(0.2);

// Directional Shadow Mapping
uniform int sampleRadius;
uniform float shadowBias;
uniform float biasVariation;
uniform sampler2D shadowMap;


// P, A, V Lights
struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
};
struct AreaLight {
    vec3 position;
};
struct VolumeLight {
    vec3 position;
};

uniform PointLight pointLight;
uniform AreaLight areaLight;
uniform VolumeLight volumeLight;

uniform sampler2D plShadowMap;



/* -------------------------- LIGHTING -------------------------- */
LightingComponents blinnPhong(vec3 N, vec3 L, vec3 V, vec3 Ka, vec3 Kd, vec3 Ks, float Ns) {
    LightingComponents components;
    vec3 H = normalize(L + V);

    components.ambient  = Ia * Ka.rgb;
    components.diffuse  = Id * Kd * max(dot(N, L), 0.0);
    components.specular = Is * Ks.rgb * pow(max(dot(N, H), 0.0), Ns);
    return components;
}

float directionalShadowMapping(vec3 N, vec3 L, vec4 posLightSpace) {
    vec3 lightCoords = posLightSpace.xyz / posLightSpace.w;
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

vec3 pointLightBrightness(PointLight light, vec3 pos) {
    vec3 diffuse = vec3(0.6);
    float distance = length(light.position - pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
    		            light.quadratic * (distance * distance));
    vec3 brightness = attenuation * diffuse;
    return brightness;
}



/* ---------------------------- MAIN ---------------------------- */
void main() {
    // G-buffers
    vec3 fragPos = texture(gPosition, texCoords).rgb;
    vec3 normal = texture(gNormal, texCoords).rgb;
    vec3 diffuse = texture(gDiffuse, texCoords).rgb;
    vec3 ambient = texture(gAmbient, texCoords).rgb;
    vec3 specular = texture(gSpecular, texCoords).rgb;

    vec3 result = vec3(0.0);

    if (displayType == 5) {
        // N, L, V in view space
        vec4 lightPosView = MV * vec4(lightPos, 1.0);
        vec4 P = MV * vec4(fragPos, 1.0);
        vec3 N = normalize(mat3(MV) * normal);
        vec3 L = normalize(lightPosView.xyz - P.xyz);
        vec3 V = normalize(-P.xyz);

        // Blinn-Phong
        LightingComponents blinnPhong = blinnPhong(N, L, V, ambient, diffuse, specular, 225);

        // Directional Shadow Mapping
        vec4 fragPosLightSpace = MDSM * vec4(fragPos, 1.0);
        float shadow = 0.0f;
        if (enableDSM) shadow = directionalShadowMapping(N, L, fragPosLightSpace);

        // Point, Area, Volume Lights
        vec3 brightness = vec3(0.0);
        if (enablePL) {
            brightness += pointLightBrightness(pointLight, fragPos);
        }

        // Output
        float diffuseFactor = (1.0f - shadow);
        result = enableBP
            ? brightness + (blinnPhong.ambient + blinnPhong.diffuse * diffuseFactor + blinnPhong.specular)
            : brightness + diffuse * diffuseFactor;
    
        // NPR
        if (enableNPR) {
            float intensity = dot(N, L);
            if (intensity > 0.66)
                result *= 1.25;
            else if (intensity > 0.33)
                result *= 1.0;
            else
                result *= 0.75;
        }
    }

    // Final color
    switch (displayType) {
        case 0:
            fragColor = vec4(normalize(fragPos) * 0.5 + 0.5, 1.0);
            break;
        case 1:
            fragColor = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
            break;
        case 2:
            fragColor = vec4(diffuse, 1.0);
            break;
        case 3:
            fragColor = vec4(ambient, 1.0);
            break;
        case 4:
            fragColor = vec4(specular, 1.0);
            break;
        case 5:
            fragColor = vec4(result, 1.0);
            break;
    }
}