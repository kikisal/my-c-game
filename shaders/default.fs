#version 440 core

out vec4 FragColor;

#define MAX_LIGHT 512

in vec3 frag_local_pos;
in vec3 frag_world_pos;
in vec3 frag_local_norm;
in vec3 frag_norm;
in vec2 frag_uv;
in vec4 frag_color;

#define LIGHT_POINT       0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT        2

struct Light {
    int   enabled;
    int   type;
    vec3  color;
    float intensity;
    vec3  dir;
    vec3  pos;
};

uniform vec3  color;
uniform int   no_color_attrib;
uniform Light lights[MAX_LIGHT];
uniform int   light_count;
uniform vec3  camera_pos;

// mesh textures
uniform sampler2D albedo_texture;
uniform sampler2D normal_map_texture;
uniform sampler2D roughness_texture;
uniform sampler2D specular_map_texture;

vec3 computeColor() {
    float ambientFactor = .1;
    float specular_map  = texture(specular_map_texture, frag_uv).r;

    // vec3 ao             = vec3(0);

    vec3 ambientColor =vec3(0.45, 0.76, 1.0);
    vec3 albedo       = vec3(texture(albedo_texture, frag_uv));
    ivec2 size        = textureSize(albedo_texture, 0);

    if (size.x == 1 && size.y == 1) {
        if (no_color_attrib == 1) {
            albedo = color;
        } else {
            albedo = vec3(frag_color);
        }
    }

    vec3 norm     = normalize(frag_norm);

    // radiance out
    vec3 Lo = vec3(0);

    for (int i = 0; i < light_count; ++i) {
        if (lights[i].enabled == 0) continue;

        vec3 lightDir     = normalize(lights[i].pos - frag_world_pos);
        float diff        = max(dot(norm, lightDir), 0.0);
        vec3 diffuse      = diff * lights[i].color;
        vec3 viewDir      = normalize(camera_pos - frag_world_pos);
        vec3 reflectDir   = reflect(-lightDir, norm);

        float spec_factor = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular     = specular_map * spec_factor * lights[i].color;

        float radiance    = 1.0 / (.001 + length(frag_world_pos - lights[i].pos)); 

        Lo += (diffuse + specular) * radiance * lights[i].intensity;
    }

    vec3 ambient    = ambientFactor * ambientColor;
    vec3 color      = (ambient + Lo) * albedo;
    return color;
}
vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 toneMapACES(vec3 color) {
    // ACES approximation
    const float exposureBias = 2.0; // tweak to taste
    color *= exposureBias;
    return clamp(RRTAndODTFit(color), 0.0, 1.0);
}

void main() {
    vec3 colorOut = computeColor();
    colorOut      = toneMapACES(colorOut);

    FragColor     = vec4(colorOut, 1.0);
}
