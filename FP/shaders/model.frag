#version 450 core

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
} fs_in;
in vec2 texCoord;
in vec3 normal;
in vec3 lightDir;
in vec3 eyeDir;

uniform int useNormalMap;
uniform int useBlinnPhong;
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

vec4 diffuse_mapping() {
    if (hasTex) {
        vec4 texel = texture(tex, texCoord);
        if (texel.a < 0.5) {
            discard;
        } 
        return texel;
    } else {
        return vec4(material.Kd.rgb, 1.0);
    }
}

//vec4 normal_mapping() {
//    vec3 V = normalize(eyeDir);
//    vec3 L = normalize(lightDir);
//    vec3 N = normalize(texture(normalMap, texCoord).rgb * 2.0 - vec3(1.0)); 
//    vec3 R = reflect(-L, N);
//
//    vec3 diffuse_albedo = texture(tex, texCoord).rgb;
//    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
//    vec3 specular_albedo = vec3(1.0);
//    vec3 specular = max(pow(dot(R, V), 20.0), 0.0) * specular_albedo;
//    
//    return vec4(diffuse + specular, 1.0);
//}
vec4 normal_mapping() {
    vec3 tangentNormal = texture(normalMap, texCoord).rgb * 2.0 - 1.0;
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(eyeDir);
    vec3 H = normalize(L + V);

    vec3 ambient = material.Ka.rgb * Ia;
    vec3 diffuse = max(dot(N, L), 0.0) * Id;
    vec3 specular = pow(max(dot(H, N), 0.0), material.Ns) * Is;

    if(hasTex) {
        diffuse *= texture(tex, texCoord).rgb;
    }

    return vec4(ambient + diffuse + specular, 1.0);
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
    vec3 Kd;
    if (hasTex) {
        Kd = textureColor.rgb;
    } else {
        Kd = material.Kd.rgb;
    }
    vec3 ambient  = material.Ka.rgb * Ia;
    vec3 diffuse  = max(dot(N, L), 0.0) * Kd * Id;
    vec3 specular = pow(max(dot(N, H), 0.0), material.Ns) * material.Ks.rgb * Is;

    // Output
    vec4 texel;    
    if(useBlinnPhong == 1) {
        texel = vec4(ambient + diffuse + specular, 1.0);
    }
    else if(useNormalMap == 1) {
        texel = normal_mapping();
    }    
    else {
        texel = diffuse_mapping();
    }
    
    fragColor = texel;
}