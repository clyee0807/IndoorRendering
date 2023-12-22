#version 420 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D originalSceneTex;
uniform sampler2D blurredBrightTex;

void main() {
    vec4 originalColor = texture(originalSceneTex, texCoords);
    vec4 brightColor = texture(blurredBrightTex, texCoords);
    fragColor = originalColor + brightColor;
}