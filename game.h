#ifndef GAME_H
#define GAME_H

#include <inttypes.h>

typedef struct Game_st Game;

struct Game_st {
    size_t    fps;
    
    // 
    uint32_t* pixels;
    size_t    display_width;
    size_t    display_height;
    
    size_t    sample_rate;
    size_t    audio_channels;
    int16_t*  audio;
};

Game game_init(void);
void game_update(void);
void game_key_up(int key);
void game_key_down(int key);

#endif // GAME_H