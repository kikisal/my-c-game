// MIT License

// Copyright (c) [year] [fullname]

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FLAG_GETTER_H
#define FLAG_GETTER_H

#include <stdbool.h>
#include <string.h>

#ifndef FLAGGET_API
#   define FLAGGET_API extern
#endif

typedef enum flag_opt_searchmode_enum {
    FLAG_OPT_SPACE_SEPARETED,
    FLAG_OPT_NO_SPACE
} flag_opt_searchmode_t;

#define FLAG_OPT_DEFAULT FLAG_OPT_SPACE_SEPARETED

FLAGGET_API void  flag_init(int argc, void** argv);
FLAGGET_API void  flag_free();
FLAGGET_API char* flag_get(char* option, flag_opt_searchmode_t mode);
FLAGGET_API char* flag_get_arg_str(int index);
FLAGGET_API char* flag_program_name();
FLAGGET_API char  flag_get_char(char* str, int idx, int len);

#ifdef FLAG_GETTER_IMPLEMENTATION

#include <stdlib.h>

#if !defined(FLAG_OFFSET_TABLE_CAPACITY)
#   define FLAG_OFFSET_TABLE_CAPACITY 1024
#endif

#if !defined(FLAG_FLAGOPT_BUFFER_SIZE)
#   define FLAG_FLAGOPT_BUFFER_SIZE   1024
#endif

typedef struct cmd_args_st     cmd_args_t;
typedef struct offset_table_st offset_table_t;

struct offset_table_st {
    flag_opt_searchmode_t mode;
    char* key;
    int offset;
};

struct cmd_args_st {
    char*   program_name;
    int     argc;
    char**  argv;

    char    flag_buffer[FLAG_FLAGOPT_BUFFER_SIZE];

    offset_table_t* offset_table;
    int             offset_count;
    int             offset_cap;
};

// private functions, DO NOT USE THEM.
static bool            flag__update_offset_table(char* option, flag_opt_searchmode_t mode, int i);
static offset_table_t* flag__find_offset(char* option, flag_opt_searchmode_t mode);

static cmd_args_t g_flag_Cmd_args_state;

FLAGGET_API char* flag_program_name() {
    return g_flag_Cmd_args_state.program_name;
}

FLAGGET_API void flag_init(int argc, void** argv) {
    g_flag_Cmd_args_state.program_name = (char*)argv[0];
    g_flag_Cmd_args_state.argv         = (char**)(argv + 1);
    g_flag_Cmd_args_state.argc         = argc - 1;

    g_flag_Cmd_args_state.offset_table = calloc(FLAG_OFFSET_TABLE_CAPACITY, sizeof(g_flag_Cmd_args_state.offset_table[0]));
    g_flag_Cmd_args_state.offset_count = 0;
    g_flag_Cmd_args_state.offset_cap   = FLAG_OFFSET_TABLE_CAPACITY;
}

FLAGGET_API void flag_free() {
    if (g_flag_Cmd_args_state.offset_table)
        free(g_flag_Cmd_args_state.offset_table);
    
    g_flag_Cmd_args_state = (cmd_args_t) {0};
}

FLAGGET_API char* flag_get(char* option, flag_opt_searchmode_t mode) {
    cmd_args_t* state = &g_flag_Cmd_args_state;
    
    if (!state->argv) return NULL;

    for (int i = 0; i < state->argc; ++i) {
        if (mode == FLAG_OPT_SPACE_SEPARETED) {
            if (strcmp(option, state->argv[i]) == 0) {

                if (!flag__update_offset_table(option, mode, i)) continue;

                return flag_get_arg_str(i + 1);
            }
        } else if (mode == FLAG_OPT_NO_SPACE) {
            
            char*  curr_flag     = state->argv[i];
            size_t opt_len       = strlen(option);
            size_t curr_flag_len = strlen(curr_flag);

            memset(state->flag_buffer, 0, FLAG_FLAGOPT_BUFFER_SIZE);
      
            int j = 0;
            for (; j < opt_len && j < curr_flag_len && j < FLAG_FLAGOPT_BUFFER_SIZE; ++j) {
                state->flag_buffer[j] = curr_flag[j];
            }
            // for safety discard last char.
            state->flag_buffer[FLAG_FLAGOPT_BUFFER_SIZE - 1]  = 0;
            
            char* opt_value = curr_flag + j;
            
            if (strcmp(state->flag_buffer, option) == 0) {
                if (flag__update_offset_table(option, mode, i))
                    return opt_value;
            }

        } else {
            return NULL;
        }
    }

    return NULL;
}

FLAGGET_API char flag_get_char(char* str, int idx, int len) {
    if (idx < 0 || idx >= len) return 0;
    return str[idx];
}

FLAGGET_API char* flag_get_arg_str(int index) {
    if (!g_flag_Cmd_args_state.argv) return NULL;

    if (index < 0 || index >= g_flag_Cmd_args_state.argc)
        return NULL;

    return g_flag_Cmd_args_state.argv[index];
}

bool flag__update_offset_table(char* option, flag_opt_searchmode_t mode, int i) {
    cmd_args_t* state = &g_flag_Cmd_args_state;    
    
    offset_table_t* curr_offset = flag__find_offset(option, mode);
    if (!curr_offset) {
        if (state->offset_count < state->offset_cap) {
            offset_table_t* old = state->offset_table;
            int offset_cap = state->offset_cap + FLAG_OFFSET_TABLE_CAPACITY;

            state->offset_table = realloc(state->offset_table, offset_cap);
            if (!state->offset_table) {
                state->offset_table = old;
            } else {
                state->offset_cap = offset_cap;
            }
        }

        if (state->offset_count < state->offset_cap) {
            state->offset_table[state->offset_count++] = (offset_table_t) {
                .key    = option,
                .offset = i,
                .mode   = mode
            };

            return true;
        }

        return false;
    }

    if (i <= curr_offset->offset)
        return false;
    
    curr_offset->offset = i;
    return true;
}

offset_table_t* flag__find_offset(char* option, flag_opt_searchmode_t mode) {
    cmd_args_t* state = &g_flag_Cmd_args_state;
    for (int i = 0; i < state->offset_count; ++i) {
        if (strcmp(state->offset_table[i].key, option) == 0 && state->offset_table[i].mode == mode) {
            return &(state->offset_table[i]);
        }
    }

    return NULL;
}

#endif // ifdef FLAG_GETTER_IMPLEMENTATION

// Example usage.
#if 0

// ----------------------------
// ------- EXAMPLE ONE --------
// ----------------------------

#define FLAG_GETTER_IMPLEMENTATION
#include "flag_getter.h"

#include <stdio.h>

int main(int argc, void** argv) {
    flag_init(argc, argv);
    
    char* dir1      = flag_get("-D", FLAG_OPT_NO_SPACE);
    char* dir2      = flag_get("-D", FLAG_OPT_NO_SPACE);
    char* dir3      = flag_get("-D", FLAG_OPT_NO_SPACE);
    char* dir4      = flag_get("-D", FLAG_OPT_NO_SPACE);

    char* pname     = flag_program_name();

    printf("program name: %s\n", pname);
    printf("dir1: %s\n", dir1);
    printf("dir2: %s\n", dir2);
    printf("dir3: %s\n", dir3);
    printf("dir4: %s\n", dir4);

    flag_free();
    return 0;
}

#endif // if 0

#if 0

// ----------------------------
// ------- EXAMPLE TWO --------
// ----------------------------
// Reading flags in a loop

#define FLAG_GETTER_IMPLEMENTATION
#include "flag_getter.h"

#include <stdio.h>

int main(int argc, void** argv) {
    flag_init(argc, argv);
    char* dir_nospaces;
    char* dir_spaces;

    printf("---- NO SPACES ARGS ----\n");
    while((dir_nospaces = flag_get("-D", FLAG_OPT_NO_SPACE))) {
        if (strlen(dir_nospaces) < 1) continue;
        printf("%s\n", dir_nospaces);
    }

    printf("---- WITH SPACES ARGS ----\n");
    while((dir_spaces = flag_get("-D", FLAG_OPT_DEFAULT))) {
        printf("%s\n", dir_spaces);
    }

    char* out_dir = flag_get("-o", FLAG_OPT_DEFAULT);
    printf("out dir: %s\n", out_dir);

    flag_free();
    return 0;
}

#endif // if 0

#endif // FLAG_GETTER_H
