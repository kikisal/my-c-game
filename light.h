#pragma once
#include <stdint.h>
#include <stdio.h> // snprintf
#include "engine_math.h"
#include "gl_gfx.h"


typedef struct Light_st Light_t;
typedef enum light_enum light_enum_t;

struct Light_st {
    int   enabled;
    int   type;
    Color color;
    float intensity;
    vec3  dir;
    vec3  pos;
    
    // store uniform locations too
    GLint  enabled_loc;
    GLint  type_loc;
    GLint  color_loc;
    GLint  intensity_loc;
    GLint  dir_loc;
    GLint  pos_loc;
}; // struct Light_st

enum light_enum {
    LIGHT_POINT,
    LIGHT_DIRECTIONAL,
    LIGHT_SPOT
};

#define MAX_LIGHTS 512
#define LIGHTAPI static

LIGHTAPI Light_t* createLight(vec3 position, Color color, light_enum_t type, GLProgram_t program);
LIGHTAPI bool     updateLight(Light_t* light, GLProgram_t program);
// LIGHTAPI void renderLight(Light_t* light, Camera_t* camera);


#ifdef LIGHT_IMPLEMENTATION

LIGHTAPI Light_t g_lightStack[MAX_LIGHTS];
size_t  g_lightCount = 0;

LIGHTAPI Light_t* createLight(vec3 position, Color color, light_enum_t type, GLProgram_t program) {
    if (g_lightCount >= MAX_LIGHTS) {
        return NULL;
    }

    Light_t light = {
        .type      = type,
        .color     = color,
        .enabled   = true,
        .pos       = position,
        .intensity = 1.0f,
        .dir       = {0.0f, 0.0f, -1.0f}
    };

    int index = g_lightCount;
    char uniformName[128];
   
    snprintf(uniformName, sizeof(uniformName), "lights[%d].enabled", index);
    light.enabled_loc      = glGetUniformLocation(program.program, uniformName);
    
    snprintf(uniformName, sizeof(uniformName), "lights[%d].type", index);
    light.type_loc      = glGetUniformLocation(program.program, uniformName);
    
    snprintf(uniformName, sizeof(uniformName), "lights[%d].color", index);
    light.color_loc = glGetUniformLocation(program.program, uniformName);
    
    snprintf(uniformName, sizeof(uniformName), "lights[%d].intensity", index);
    light.intensity_loc = glGetUniformLocation(program.program, uniformName);
    
    snprintf(uniformName, sizeof(uniformName), "lights[%d].dir", index);
    light.dir_loc = glGetUniformLocation(program.program, uniformName);
    
    snprintf(uniformName, sizeof(uniformName), "lights[%d].pos", index);
    light.pos_loc = glGetUniformLocation(program.program, uniformName);
    
    g_lightStack[g_lightCount++] = light;
    Light_t* l_out = &(g_lightStack[g_lightCount - 1]);
    
    updateLight(l_out, program);

    return l_out;
}


LIGHTAPI bool updateLight(Light_t* light, GLProgram_t program) {
    if (!program.program) return false;
    
    // Update the light data in the GPU
    
    glUseProgram(program.program);
    if (light->enabled_loc != -1)
        glUniform1i(light->enabled_loc, light->enabled);
    if (light->type_loc != -1)
        glUniform1i(light->type_loc, light->type);
    if (light->color_loc != -1)
        glUniform3f(light->color_loc, light->color.r, light->color.g, light->color.b);
    if (light->intensity_loc != -1)
        glUniform1f(light->intensity_loc, light->intensity);
    if (light->dir_loc != -1)
        glUniform3f(light->dir_loc, light->dir.x, light->dir.y, light->dir.z);
    if (light->pos_loc != -1)
        glUniform3f(light->pos_loc, light->pos.x, light->pos.y, light->pos.z);
    

    glUniform1i(program.light_count_loc, g_lightCount);
    glUseProgram(0);

    return true;
}
#endif // LIGHT_IMPLEMENTATION