#version 450 core

layout(location = 0) in vec3 inPos;

uniform mat4 MshadowMap;

void main() {
    gl_Position = MshadowMap * vec4(inPos, 1.0);
}