#version 450 core
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D screenTex;


void main()
{
    // convolution for detecting x & y edge
    float kernel_x[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
    float kernel_y[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

    vec2 texSize = 1.0 / textureSize(screenTex, 0);

    vec2 offsets[9] = {
        vec2(-1, -1), vec2(0, -1), vec2(1, -1),
        vec2(-1,  0), vec2(0,  0), vec2(1,  0),
        vec2(-1,  1), vec2(0,  1), vec2(1,  1)
    };

    vec3 grad_x = vec3(0);
    vec3 grad_y = vec3(0);
    for(int i=0; i<9; i++){
        vec2 offset = offsets[i] * texSize;
        vec3 samples = texture(screenTex, texCoord + offset).rgb;
        grad_x += samples * kernel_x[i];
        grad_y += samples * kernel_y[i];
    }
    
    grad_x = abs(grad_x), grad_y = abs(grad_y);
    float edge_x = grad_x.x + grad_x.y + grad_x.z;
    float edge_y = grad_y.x + grad_y.y + grad_y.z;

    if (edge_x >= 0.8 || edge_y >= 0.8) {  // threshold = 0.7, black edge
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } 
    else {
        fragColor = texture(screenTex, texCoord);
    }
}
