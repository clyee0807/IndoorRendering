#version 450 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

out vec4 fragPos;

uniform mat4 MPLS[6];

void main() {             
	for(int face = 0; face < 6; face++) {
        gl_Layer = face; // Built-in variable that specifies to which face we render.

        // For each triangle vertex
        for(int i = 0; i < 3; i++) {
            fragPos = gl_in[i].gl_Position;
            gl_Position = MPLS[face] * fragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}