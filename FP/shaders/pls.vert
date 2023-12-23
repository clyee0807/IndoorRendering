#version 450 core

layout(location = 0) in vec3 inPos;

uniform mat4 MM;

void main() {
    gl_Position = MM * vec4(inPos, 1.0);
}