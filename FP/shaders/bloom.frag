#version 420 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D originalSceneTex;
uniform sampler2D blurredBrightTex;

void main() {
    vec4 originalColor = texture(originalSceneTex, texCoord);
    vec4 brightColor = texture(blurredBrightTex, texCoord);
    fragColor = originalColor + brightColor;
}