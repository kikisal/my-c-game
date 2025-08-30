#pragma once

#include <inttypes.h>
#include <stdbool.h>


#define SAMPLE_RATE       44100
#define AUDIO_CHANNELS    2

typedef struct AudioDevice_st AudioDevice;

struct AudioBuffer_st {
    int16_t*     buffer;
    size_t       size;
    AudioDevice* device;
    size_t       time;
    bool         playing;
}; // struct AudioBuffer

typedef struct AudioBuffer_st AudioBuffer;
typedef struct AudioMixer_st  AudioMixer;

struct AudioMixer_st {
    AudioDevice*  device;
    AudioBuffer** audio_list;
    size_t        count;
    float*        mx_buff;
};

extern AudioDevice* audio_init_device();

// audio buffer
extern AudioBuffer  audio_buffer_create(size_t sample_count, AudioDevice* device);
extern float        audio_buffer_lc_sample_flt(AudioBuffer* ab, size_t time);
extern float        audio_buffer_rc_sample_flt(AudioBuffer* ab, size_t time);
extern float        audio_buffer_sample_flt(AudioBuffer* ab, size_t ch, size_t time);

extern void         audio_buffer_free(AudioBuffer ab);
extern void         audio_buffer_update(AudioBuffer* audio);
extern void         audio_buffer_play(AudioBuffer* audio);
extern void         audio_buffer_pause(AudioBuffer* audio);
extern void         audio_buffer_seek_start(AudioBuffer* audio);
extern void         audio_buffer_seek(AudioBuffer* audio, size_t time);

extern void         audio_buffer_sin_fill_mono(AudioBuffer buff);
extern void         audio_buffer_sin_fill_stereo(AudioBuffer buff);
extern void         audio_buffer_sin_fill_stereo_low(AudioBuffer buff);


// audio mixer
extern bool        audio_mixer_create(AudioMixer* mixer_out, size_t capacity, AudioDevice* device);
extern void        audio_mixer_update(AudioMixer* mixer);

// audio utilities
extern int16_t      float_to_int16(float f);
extern float        int16_to_float(int16_t v);


#ifdef AUDIO_IMPLEMENTATION

#ifndef M_PI
#   define M_PI 3.14159265
#endif

static AudioDevice* s_audioDevice = NULL;

#define AUDIO_SAMPLES_PER_FRAME SAMPLE_RATE / FPS 

#define AUDIO_BUFFERS      4
#define AUDIO_BUFFERS_SIZE 2 * AUDIO_SAMPLES_PER_FRAME

static int16_t* s_audioBuffers[AUDIO_BUFFERS];

#if defined(_WIN32)

static int     s_Audio_CurrentWHDR = 0;
static WAVEHDR s_Audio_WinHDRS[AUDIO_BUFFERS];

void CALLBACK audio_event_handler_win(HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

AudioBuffer audio_buffer_create(size_t sample_count, AudioDevice* device) {
    AudioBuffer ab = (AudioBuffer) {
        .buffer  = NULL,
        .size    = sample_count,
        .device  = device == NULL ? s_audioDevice : device,
        .time    = 0,
        .playing = false
    };

    ab.buffer = malloc(sample_count * sizeof(*ab.buffer));
    if (!ab.buffer) return (AudioBuffer) {0};
    
    return ab;
}

float audio_buffer_lc_sample_flt(AudioBuffer* ab, size_t time) {
    float sample  = 0.0f;

    size_t sample_idx = 2 * time;
    if (sample_idx < ab->size)
        sample  = int16_to_float(ab->buffer[sample_idx + 0]);

    return sample;
}


float audio_buffer_rc_sample_flt(AudioBuffer* ab, size_t time) {
    float sample  = 0.0f;

    size_t sample_idx = 2 * time;
    if (sample_idx < ab->size)
        sample  = int16_to_float(ab->buffer[sample_idx + 1]);

    return sample;
}

float audio_buffer_sample_flt(AudioBuffer* ab, size_t ch, size_t time) {
    float sample  = 0.0f;

    size_t sample_idx = AUDIO_CHANNELS * time;
    if (sample_idx < ab->size)
        sample  = int16_to_float(ab->buffer[sample_idx + ch]);

    return sample;
}

void audio_buffer_free(AudioBuffer ab) {
    if (ab.buffer) free(ab.buffer);
}

void audio_buffer_seek_start(AudioBuffer* audio) {
    if (!audio) return;
    audio->time = 0;
}

void audio_buffer_seek(AudioBuffer* audio, size_t time) {
    if (!audio) return;
    audio->time = time;
}


bool audio_mixer_create(AudioMixer* mixer_out, size_t capacity, AudioDevice* device) {
    if (!mixer_out) return false;
    
    mixer_out->count      = capacity;
    mixer_out->audio_list = malloc(sizeof(mixer_out->audio_list[0]) * capacity);
    mixer_out->device     = device != NULL ? device : s_audioDevice;
    mixer_out->mx_buff    = malloc(sizeof(mixer_out->mx_buff[0]) * AUDIO_BUFFERS_SIZE * AUDIO_CHANNELS);

    return mixer_out->audio_list != NULL && mixer_out->mx_buff != NULL;
}

void audio_mixer_update(AudioMixer* mixer) {
    WAVEHDR* whdr         = &(s_Audio_WinHDRS[s_Audio_CurrentWHDR]);
    HWAVEOUT device       = (HWAVEOUT)mixer->device;

    while (whdr->dwFlags & WHDR_INQUEUE) { Sleep(1); }

    int16_t* winAudioBuffer = (int16_t*) whdr->lpData;

    // -- Step 1: Mix signals along time dimension t --
    for (size_t t = 0; t < AUDIO_BUFFERS_SIZE; ++t) {
        float mixed[AUDIO_CHANNELS]  = {0.0f};

        for (size_t i = 0; i < mixer->count; ++i) {
            AudioBuffer* ab = mixer->audio_list[i];
            
            if (!ab->playing) continue;

            for (int c = 0; c < AUDIO_CHANNELS; ++c) {
                mixed[c] += audio_buffer_sample_flt(ab, c, ab->time + t);
            }
        }
        
        for (size_t c = 0; c < AUDIO_CHANNELS; ++c) {
            winAudioBuffer[AUDIO_CHANNELS*t + c] = float_to_int16(mixed[c]);
        }
    }

    // -- Step 2: Normalize mixer->mx_buff --
    // float sum[AUDIO_CHANNELS] = {0.0f};

    // for (size_t t = 0; t < AUDIO_BUFFERS_SIZE; ++t) {
    //     for (size_t c = 0; c < AUDIO_CHANNELS; ++c) {
    //         sum[c] += mixer->mx_buff[AUDIO_CHANNELS*t + c];
    //     }
    // }

    // float eps = 0.0001f;
    // for (size_t t = 0; t < AUDIO_BUFFERS_SIZE; ++t) {
    //     for (size_t c = 0; c < AUDIO_CHANNELS; ++c) {
    //         winAudioBuffer[AUDIO_CHANNELS*t + c] = float_to_int16(mixer->mx_buff[AUDIO_CHANNELS*t + c] * (1.0f / (sum[c] + eps)));
    //     }
    // }

    // advance audio time
    for (size_t i = 0; i < mixer->count; ++i) {
        if (mixer->audio_list[i]->playing)
            mixer->audio_list[i]->time += AUDIO_BUFFERS_SIZE;
    }


    whdr->dwBufferLength = AUDIO_BUFFERS_SIZE * AUDIO_CHANNELS *  sizeof(int16_t);
    waveOutWrite(device, whdr, sizeof(WAVEHDR));
    s_Audio_CurrentWHDR = (s_Audio_CurrentWHDR + 1) % AUDIO_BUFFERS;
}

void audio_buffer_update(AudioBuffer* audio) {
    if (!audio || !audio->playing) return;

    if (audio->time * AUDIO_CHANNELS>= audio->size)
        return;

    size_t audio_samples = AUDIO_BUFFERS_SIZE;
    if (2*(audio->time + audio_samples) >= audio->size)
        audio_samples = (audio->size/AUDIO_CHANNELS) - audio->time;

    // Prepare buffer
    WAVEHDR* whdr   = &(s_Audio_WinHDRS[s_Audio_CurrentWHDR]);
    HWAVEOUT device = (HWAVEOUT)audio->device;

    while (whdr->dwFlags & WHDR_INQUEUE) { Sleep(1); }

    memcpy(whdr->lpData, audio->buffer + AUDIO_CHANNELS * audio->time, audio_samples * AUDIO_CHANNELS * sizeof(int16_t));
    
    whdr->dwBufferLength  = AUDIO_CHANNELS * audio_samples * sizeof(int16_t);

    waveOutWrite(device, whdr, sizeof(WAVEHDR));
    audio->time += audio_samples;

    s_Audio_CurrentWHDR = (s_Audio_CurrentWHDR + 1) % AUDIO_BUFFERS;
}

void audio_buffer_play(AudioBuffer* audio) {
    if (!audio->device) return;
    audio->playing = true;
}

void audio_buffer_pause(AudioBuffer* audio) {
    if (!audio->device) return;
    audio->playing = false;
}

#if defined(_WIN32)

AudioDevice* audio_init_device() {
    // Open audio device
    HWAVEOUT hWaveOut;
    WAVEFORMATEX wfx    = {0};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = AUDIO_CHANNELS;
    wfx.nSamplesPerSec  = SAMPLE_RATE;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)audio_event_handler_win, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        printf("Failed to open audio device.\n");
        return NULL;
    }

    s_audioDevice = (AudioDevice*) hWaveOut;

    for (int i = 0; i < AUDIO_BUFFERS; ++i) {
        s_audioBuffers[i]                 = calloc(AUDIO_BUFFERS_SIZE * AUDIO_CHANNELS, sizeof(int16_t));

        s_Audio_WinHDRS[i].lpData         = (LPSTR) s_audioBuffers[i];
        s_Audio_WinHDRS[i].dwBufferLength = AUDIO_BUFFERS_SIZE * AUDIO_CHANNELS * sizeof(int16_t);
        s_Audio_WinHDRS[i].dwFlags        = 0;

        waveOutPrepareHeader((HWAVEOUT)s_audioDevice, &s_Audio_WinHDRS[i], sizeof(WAVEHDR));
    }

    return (AudioDevice*) hWaveOut;
}
#else
#   error "audio_init_device() not implemented for current OS."
#endif // if defined(_WIN32)

void audio_buffer_sin_fill_stereo_low(AudioBuffer buff) {
    float Dt        = 1.0f / SAMPLE_RATE;
    size_t samples  = buff.size / 2;

    for (size_t i = 0; i < samples; ++i) {
        buff.buffer[2*i + 0] = float_to_int16(.5f * sin(2 * M_PI * i * Dt * 3*130.81f));
        buff.buffer[2*i + 1] = float_to_int16(.5f * sin(2 * M_PI * i * Dt * 3*130.81f));
    }
}

void audio_buffer_sin_fill_stereo(AudioBuffer buff) {
    float Dt        = 1.0f / SAMPLE_RATE;
    size_t samples  = buff.size / 2;

    int left = 1;
    for (size_t i = 0; i < samples; ++i) {
        if (i % 10000 == 0 && i > 0) { left = !left; }

        if (left) {
            buff.buffer[2*i + 0] = float_to_int16(.33333f * sin(2 * M_PI * i * Dt * 261.6f));
            buff.buffer[2*i + 1] = 0;
        } else {
            buff.buffer[2*i + 0] = 0;
            buff.buffer[2*i + 1] = float_to_int16(.33333f * sin(2 * M_PI * i * Dt * 2*261.6f));
        }
    }
}

void audio_buffer_sin_fill_mono(AudioBuffer buff) {
    float Dt = 1.0f / SAMPLE_RATE;

    for (size_t i = 0; i < buff.size / 2; ++i) {
        buff.buffer[i] = float_to_int16(.33333f * sin(2 * M_PI * i * Dt * 261.6f));
        if (i >= 10000) {
            buff.buffer[i] = buff.buffer[i] + float_to_int16(0.33333f * sin(2 * M_PI * i * Dt * 329.63f));
        }

        if (i >= 20000)
            buff.buffer[i] = buff.buffer[i] + float_to_int16(0.33333f * sin(2 * M_PI * i * Dt * 392.0f));
    }

    for (size_t i = 0; i < buff.size / 2; ++i) {
        buff.buffer[i + buff.size / 2] = float_to_int16(1.0f * sin(2 * M_PI * i * Dt * 2*261.6f));
    }
}

float int16_to_float(int16_t v)
{
    return ((float) v) / 32767.0f;
}

int16_t float_to_int16(float f)
{
    // clamp to [-1,1] just in case
    if (f > 1.0f)
        f = 1.0f;
    if (f < -1.0f)
        f = -1.0f;

    return (int16_t)(f * 32767.0f);
}

#if defined(_WIN32)
void CALLBACK audio_event_handler_win(
    HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2) 
{
    switch (uMsg) {
        case WOM_DONE: {
            WAVEHDR* pHdr = (WAVEHDR*)dwParam1;

            pHdr->dwFlags &= ~WHDR_INQUEUE;
            break;
        }
    }
}

#endif

#endif // AUDIO_IMPLEMENTATION