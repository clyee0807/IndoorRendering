#version 420 core

uniform sampler2D screenTex;
uniform int pixelSize;

in vec2 texCoord;
out vec4 fragColor;

void main() {
    vec2 gridSize = vec2(pixelSize) / vec2(textureSize(screenTex, 0));
    
    vec2 gridOrigin = floor(texCoord / gridSize) * gridSize;
    
    vec4 colorSum = vec4(0.0);
    float count = 0.0;
    for (int x = 0; x < pixelSize; ++x) {
        for (int y = 0; y < pixelSize; ++y) {
            vec2 offset = vec2(x, y) / vec2(textureSize(screenTex, 0));
            colorSum += texture(screenTex, gridOrigin + offset);
            count += 1.0;
        }
    }

    fragColor = colorSum / count;
}