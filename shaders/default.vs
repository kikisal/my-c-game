#version 440 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec4 aColor;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

out vec3 frag_local_pos;
out vec3 frag_world_pos;
out vec3 frag_local_norm;
out vec3 frag_norm;
out vec2 frag_uv;
out vec4 frag_color;

void main() {
    mat4 mvp        = proj_mat * view_mat * world_mat;
    gl_Position     = mvp * vec4(aPos, 1.0);
    
    frag_local_pos  = aPos;
    frag_world_pos  = vec3(world_mat * vec4(aPos, 1.0));
    frag_local_norm = aNormal;
    frag_norm       = mat3(world_mat) * aNormal;
    frag_uv         = aUv;
    frag_color      = aColor;
}