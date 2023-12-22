#version 450 core

layout(location = 0) out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D currentTex;

void main() {
    fragColor = texture(currentTex, texCoords);
}