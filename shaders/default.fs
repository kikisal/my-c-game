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

uniform int hasTangentAttrib;
in mat3 TBN;

in vec3 tangent_out;
in vec3 bitangent_out;

bool validTexture(sampler2D texture) {
    ivec2 size  = textureSize(texture, 0);
    return size.x != 1 && size.y != 1;
}

vec3 computeRadiance(Light light, vec3 norm, float radiance, vec3 lightDir, vec3 viewDir, float specular, float roughness) {
    float diff         = max(dot(norm, lightDir), 0.0);
    vec3  diffuse      = diff * light.color;
    vec3  reflectDir   = reflect(-lightDir, norm);

    float shininess = 1.0 - roughness;
    
    vec3  halfwayDir    = normalize(lightDir + viewDir);
    float spec_factor   = pow(max(dot(norm, halfwayDir), 0.0), shininess * 128);
    vec3  spec          = specular * spec_factor * light.color;

    return (diffuse + spec) * radiance * light.intensity;
    // return spec;
}

vec3 computeDirectionalLight(Light light, vec3 normal, float specular, float roughness) {
    vec3  viewDir      = normalize(camera_pos - frag_world_pos);
    return computeRadiance(light, normal, 1.0, -light.dir, viewDir, specular, roughness);
}

vec3 computeSpotLight(Light light, vec3 normal, float specular, float roughness) {
    return vec3(0.0);
}

vec3 computePointLight(Light light, vec3 normal, float specular, float roughness) {
    vec3  lightDir     = normalize(light.pos - frag_world_pos);
    float radiance     = 1.0 / (1.0 + length(frag_world_pos - light.pos)); 
    vec3  viewDir      = normalize(camera_pos - frag_world_pos);

    return computeRadiance(light, normal, radiance, lightDir, viewDir, specular, roughness);
}

vec3 computeColor() {
    
    vec3 ambientColor = vec3(0.529, 0.808, 0.980);
    // vec3 ao             = vec3(0);
    float ambientFactor = .4;

    float specular  = texture(specular_map_texture, frag_uv).r;
    float roughness = .2;
    vec3 normal     = frag_norm;
    vec3 albedo     = vec3(texture(albedo_texture, frag_uv));

    if (!validTexture(albedo_texture))
        albedo = no_color_attrib == 1 ? color : vec3(frag_color);

    if (!validTexture(specular_map_texture)) {
        specular = 0.5;
    }

    if (validTexture(normal_map_texture) && hasTangentAttrib == 1) {
        normal = vec3(texture(normal_map_texture, frag_uv));
        // tangent space normal
        normal = TBN * normalize(normal * 2.0 - 1.0);
    }

    // radiance out
    vec3 Lo = vec3(0);

    for (int i = 0; i < light_count; ++i) {
        if (lights[i].enabled == 0) continue;

        switch(lights[i].type) {
            case LIGHT_POINT:
                Lo += computePointLight(lights[i], normal, specular, roughness);
                break;
            case LIGHT_DIRECTIONAL:
                Lo += computeDirectionalLight(lights[i], normal, specular, roughness);
                break;
            case LIGHT_SPOT:
                Lo += computeSpotLight(lights[i], normal, specular, roughness);
                break;
        }
    }

    vec3 ambient    = ambientFactor * ambientColor;
    vec3 color      = (ambient + Lo) * albedo;
    return color;
}


void main() {
    vec3 colorOut = computeColor();

    FragColor     = vec4(colorOut, 1.0);
}
