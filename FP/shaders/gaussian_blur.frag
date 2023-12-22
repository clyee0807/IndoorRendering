#version 420 core

uniform sampler2D brightTex;
uniform bool isHorizontal;
uniform float sigma; // The standard deviation of the blur

in vec2 texCoords;
out vec4 fragColor;

const int kernelSize = 11; // Number of samples in one direction; total samples = kernelSize^2
const float pi = 3.14159265;

// Function to compute the weight of a Gaussian distribution at a certain distance from the mean
float gaussian(float x, float sigma) {
    return exp(-0.5 * x * x / (sigma * sigma)) / (2.0 * pi * sigma * sigma);
}

void main() {
    vec2 tex_offset = 1.0 / textureSize(brightTex, 0); // Gets the size of one texel
    float totalWeight = 0.0; // Keep track of total weight for normalization
    vec4 blur = vec4(0.0);

    // Sum the contributions of the texels in the kernel
    for (int i = -kernelSize / 2; i <= kernelSize / 2; ++i) {
        float weight = gaussian(float(i), sigma);
        vec2 offset = isHorizontal ? vec2(tex_offset.x * float(i), 0.0) : vec2(0.0, tex_offset.y * float(i));
        blur += texture(brightTex, texCoords + offset) * weight;
        totalWeight += weight;
    }

    fragColor = blur / totalWeight; // Normalize the color by the total weight
}