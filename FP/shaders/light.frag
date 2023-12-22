#version 450 core

layout(location = 0) out vec4 fragColor;

in vec2 texCoord;

// Shading
uniform bool hasTex;
uniform sampler2D tex;

// Shading
uniform int lightType;

// Lighting
struct Material {
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
    float Ns;
};
struct PointLight {
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

uniform Material material;
uniform PointLight pointLight;
uniform AreaLight areaLight;
uniform VolumeLight volumeLight;

void main() {     
    vec4 textureColor = texture(tex, texCoord);
	vec3 Kd = hasTex ? textureColor.rgb : material.Kd.rgb;

    switch (lightType) {
        case 0:  // Point
            break;

        case 1:  // Area
            break;

        case 2:  // Volume
            break;
    }

    fragColor = vec4(Kd, 1.0);
}