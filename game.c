#include "game.h"
#include <assert.h>

#define DISPLAY_WIDTH     800
#define DISPLAY_HEIGHT    600
#define GAME_FPS          60
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS    2

static_assert(AUDIO_SAMPLE_RATE%GAME_FPS == 0, "Sample rate must be divisible by FPS");

#define AUDIO_SAMPLES     (AUDIO_SAMPLE_RATE/GAME_FPS) * AUDIO_CHANNELS

static uint32_t g_Pixels    [DISPLAY_WIDTH * DISPLAY_HEIGHT];
static int16_t g_audioBuffer[AUDIO_SAMPLES];

Game game_init(void) {
    return (Game) {
        .fps            = GAME_FPS,
        .pixels         = g_Pixels,
        .display_width  = DISPLAY_WIDTH,
        .display_height = DISPLAY_HEIGHT,
        .sample_rate    = AUDIO_SAMPLE_RATE,
        .audio_channels = AUDIO_CHANNELS,
        .audio          = g_audioBuffer,
    };
}

void game_update(void) {
    TODO("game_update");
}

void game_key_up(int key) {
    TODO("game_key_up");
}

void game_key_down(int key) {
    TODO("game_key_down");
}