#version 450 core

layout(location = 0) in vec3 inPos;

uniform mat4 MDSM;

void main() {
    gl_Position = MDSM * vec4(inPos, 1.0);
}