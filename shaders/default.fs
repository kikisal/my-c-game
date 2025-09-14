#version 440 core

out vec4 FragColor;

in vec3 frag_local_pos;
in vec3 frag_world_pos;
in vec3 frag_local_norm;
in vec3 frag_norm;
in vec2 frag_uv;
in vec4 frag_color;

void main() {
    // float ambientFactor = .1;
    // vec3 ambient  = ambientFactor * lightColor;
    FragColor = vec4(vec3(frag_color), 1.0);
}
