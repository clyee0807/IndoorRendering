#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inTex;

out vec2 texCoords;

// MVP
uniform mat4 MM;
uniform mat4 MV;
uniform mat4 MP;

void main() {
    texCoords = inTex;
    gl_Position = MP * MV * MM * vec4(inPos, 1.0);
}