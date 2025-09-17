#pragma once

#include "deps/glad/glad.h"
#include "file.h"


#define ATTRIB_POSITION_LOCATION    0
#define ATTRIB_NORMAL_LOCATION      1
#define ATTRIB_UV_LOCATION          2
#define ATTRIB_COLOR_LOCATION       3

#define UNIFORM_WORLD_MATRIX   "world_mat"
#define UNIFORM_VIEW_MATRIX    "view_mat"
#define UNIFORM_PROJ_MATRIX    "proj_mat"
#define UNIFORM_LIGHT_COUNT    "light_count"
#define UNIFORM_NOCOLOR_ATTRIB "no_color_attrib"
#define UNIFORM_COLOR_LOC      "color"

typedef struct GLProgram_st GLProgram_t;

struct GLProgram_st {
    GLuint program;

    // unifroms
    GLint world_mat_loc;
    GLint view_mat_loc;
    GLint proj_mat_loc;
    GLint light_count_loc;

    GLint no_color_attrib_loc;
    GLint color_loc;
};

enum {
    TEXTURE_ALBEDO_TEXTURE,
    TEXTURE_NORMAL_MAP_TEXTURE,
    TEXTURE_ROUGHNESS_TEXTURE,
    TEXTURE_SPECULAR_MAP,
    TEXTURE_COUNT
};

const char* UNIFORM_TEXTURE_NAMES[TEXTURE_COUNT] = {
    "albedo_texture",
    "normal_map_texture",
    "roughness_texture",
    "specular_map_texture",
};

struct Texture_st {
    GLuint texture_id;
};

typedef struct Texture_st Texture_t;

void        canvas_to_GLtexture(Olivec_Canvas src, GLint dest);
GLuint      createShaderProgramGL_(File_t vs_file, File_t fs_file);
GLProgram_t createShaderProgramGL(File_t vs_file, File_t fs_file);

#ifdef GLGFX_IMPLEMENTATION

GLuint createShaderProgramGL_(File_t vs_file, File_t fs_file) {
    GLuint vs = compile_shader(vs_file.data, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_file.data, GL_FRAGMENT_SHADER);
    GLuint quadProg = glCreateProgram();
    glAttachShader(quadProg, vs);
    glAttachShader(quadProg, fs);
    glLinkProgram(quadProg);
    return quadProg;
}

GLProgram_t createShaderProgramGL(File_t vs_file, File_t fs_file) {
    GLuint programID = createShaderProgramGL_(vs_file, fs_file);
    GLProgram_t program = {0};

    if (!programID) return program;

    program.program         = programID;

    // load uniforms
    program.world_mat_loc       = glGetUniformLocation(programID, UNIFORM_WORLD_MATRIX);
    program.view_mat_loc        = glGetUniformLocation(programID, UNIFORM_VIEW_MATRIX);
    program.proj_mat_loc        = glGetUniformLocation(programID, UNIFORM_PROJ_MATRIX);
    program.light_count_loc     = glGetUniformLocation(programID, UNIFORM_LIGHT_COUNT);
    program.no_color_attrib_loc = glGetUniformLocation(programID, UNIFORM_NOCOLOR_ATTRIB);
    program.color_loc           = glGetUniformLocation(programID, UNIFORM_COLOR_LOC);
    
    return program;
}

void canvas_to_GLtexture(Olivec_Canvas src, GLint dest) {
    glBindTexture(GL_TEXTURE_2D, dest);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src.width, src.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, src.pixels);
    glBindTexture(GL_TEXTURE_2D, dest);    
}

#endif // GLGFX_IMPLEMENTATION