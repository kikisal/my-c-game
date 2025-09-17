#ifndef GAME_H
#define GAME_H

#define OLIVECDEF extern
#include "deps/olivec/olive.c"
#include <inttypes.h>

#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 4

#define GAME_TITLE              "Chronicles of Zarathia"
#define CANVAS_WIDTH            (16 * 80)
#define CANVAS_HEIGHT           (9  * 80)

#define AUDIO_CHANNELS          2
#define SAMPLE_RATE             44100
#define FPS                     60

void game_init(Olivec_Canvas canvas);
void game_update(Olivec_Canvas canvas);
void game_close(void);
void game_key_up(int key);
void game_key_down(int key);

#endif // GAME_H