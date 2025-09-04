#version 440 core

out vec4 FragColor;



in vec3 frag_local_pos;
in vec3 frag_world_pos;
in vec3 frag_local_norm;
in vec3 frag_norm;
in vec2 frag_uv;
in vec4 frag_color;


void main() {
    FragColor = vec4(frag_uv, 0.0, 1.0);
}
