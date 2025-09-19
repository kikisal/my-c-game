#version 440 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec4 aColor;
layout(location = 4) in vec3 aTangent;

uniform int hasTangentAttrib;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

out vec3 frag_local_pos;
out vec3 frag_world_pos;
out vec3 frag_local_norm;
out vec3 frag_ts_pos; // fragment tangent pos

out vec3 frag_norm;
out vec3 frag_tangent;
out vec3 frag_bitangent;

out vec2 frag_uv;
out vec4 frag_color;

out mat3 TBN;
out vec3 tangent_out;
out vec3 bitangent_out;

void main() {
    mat4 mvp        = proj_mat * view_mat * world_mat;
    gl_Position     = mvp * vec4(aPos, 1.0);
    
    frag_local_pos  = aPos;
    frag_world_pos  = vec3(world_mat * vec4(aPos, 1.0));
    frag_local_norm = aNormal;
    frag_norm       = normalize(mat3(transpose(inverse(world_mat))) * aNormal);
    
    frag_uv         = aUv;
    frag_color      = aColor;

    if (hasTangentAttrib == 1) {
        vec3 T        = normalize(mat3(transpose(inverse(world_mat))) * aTangent);
        vec3 N        = frag_norm;
        // re-orthogonalization
        T             = normalize(T - dot(T, N) * N);
        vec3 B        = cross(N, T);
        TBN           = mat3(T, B, N);
        tangent_out   = T;
        bitangent_out = B;
    }
}