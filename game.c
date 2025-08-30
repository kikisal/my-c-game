#include "game.h"
#include "audio.h"
#include <assert.h>
#include <stdio.h>
#include "deps/olivec/olive.c"

float x = 10.0f;
float y = 300.0f;

float speedX = 100.f;
float speedY = -100.f;

AudioBuffer audio;
AudioBuffer bg_song;
AudioMixer mixer;

void game_init(Olivec_Canvas canvas) {
    audio   = audio_buffer_create(SAMPLE_RATE / 2  * AUDIO_CHANNELS, NULL);
    bg_song = audio_buffer_create(SAMPLE_RATE * 10 * AUDIO_CHANNELS, NULL);

    // mixer usage example:
    audio_mixer_create(&mixer, 2, NULL);

    // -- Add as many audio tracks as you want in the mixer. --
    mixer.audio_list[0] = &audio;
    mixer.audio_list[1] = &bg_song;

    audio_buffer_sin_fill_stereo(audio);
    audio_buffer_sin_fill_stereo_low(bg_song);

    audio_buffer_play(&bg_song);
    
    // x = canvas.width - 40;
    // y = 0;
}

void game_update(Olivec_Canvas canvas) {

    olivec_rect(canvas, (int)x, (int)y, 40, 40, 0xFFFF0000);

    x += (speedX * 1.0f/60.0f);
    y += (speedY * 1.0f/60.0f);

    if (y <= 0 || y + 40 >= CANVAS_HEIGHT) speedY = -speedY;
    if (y <= 0) y = 0;
    if (y + 40 >= CANVAS_HEIGHT) y = CANVAS_HEIGHT - 40;

    if (x <= 0 || x + 40 >= CANVAS_WIDTH) speedX = -speedX;
    if (x <= 0) x = 0;

    if (x + 40 >= CANVAS_WIDTH) x = CANVAS_WIDTH - 40;

    
    if (y <= 0 || y + 40 >= CANVAS_HEIGHT || x <= 0 || x + 40 >= CANVAS_WIDTH) {
        audio_buffer_seek_start(&audio);
        audio_buffer_play(&audio);
    }

    audio_mixer_update(&mixer);
}

void game_close() {
    audio_buffer_free(audio);
}

void game_key_up(int key) {
    // TODO("game_key_up");
}

void game_key_down(int key) {
    // TODO("game_key_down");
}