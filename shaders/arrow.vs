#version 440 core

layout(location = 0) in vec3 aPos;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    mat4 mvp        = proj_mat * view_mat * world_mat;
    gl_Position     = mvp * vec4(aPos, 1.0);
}