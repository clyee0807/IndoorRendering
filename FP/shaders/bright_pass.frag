#version 420 core

uniform sampler2D sceneTex;
uniform float brightnessThreshold;

in vec2 texCoords;
out vec4 fragColor;

void main() {
    vec4 color = texture(sceneTex, texCoords);

    // Luminance
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > brightnessThreshold) {
        fragColor = color;
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}