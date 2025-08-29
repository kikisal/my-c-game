#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#ifndef M_PI
#   define M_PI 3.14159265
#endif

#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100

typedef struct AudioDevice_st AudioDevice;

struct AudioBuffer_st {
    int16_t*     buffer;
    size_t       size;
    AudioDevice* device;
}; // struct AudioBuffer

typedef struct AudioBuffer_st AudioBuffer;

int16_t float_to_int16(float f);
AudioBuffer audio_buffer_create(size_t sample_count, AudioDevice* device);
void audio_buffer_free(AudioBuffer ab);
void audio_buffer_play(AudioBuffer audio);

#define PLATFORM_WINDOWS

#ifdef PLATFORM_WINDOWS
void CALLBACK audio_event_handler_win(HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

// fills buffer with sin function samples.
void audio_buffer_sin_fill_mono(AudioBuffer buff);
void audio_buffer_sin_fill_stereo(AudioBuffer buff);

int main() {

    // Open audio device
    HWAVEOUT hWaveOut;
    WAVEFORMATEX wfx    = {0};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 2;                // mono
    wfx.nSamplesPerSec  = SAMPLE_RATE;      // sample rate
    wfx.wBitsPerSample  = 16;               // int16_t = 16 bits
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)audio_event_handler_win, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        printf("Failed to open audio device.\n");
        return 1;
    }

    AudioBuffer audio = audio_buffer_create(SAMPLE_RATE * 10 * 2, (AudioDevice*)hWaveOut);
    audio_buffer_sin_fill_stereo(audio);

    audio_buffer_play(audio);

    getchar();
    printf("finished!\n");
    waveOutClose(hWaveOut);

    audio_buffer_free(audio);

    return 0;
}


AudioBuffer audio_buffer_create(size_t sample_count, AudioDevice* device) {
    AudioBuffer ab = (AudioBuffer) {
        .buffer  = NULL,
        .size    = sample_count,
        .device  = device
    };

    ab.buffer = malloc(sample_count * sizeof(*ab.buffer));
    if (!ab.buffer) return (AudioBuffer) {0};
    
    return ab;
}

void audio_buffer_free(AudioBuffer ab) {
    if (ab.buffer) free(ab.buffer);
}

void audio_buffer_play(AudioBuffer audio) {
    if (!audio.device) return;

    // Prepare buffer
    WAVEHDR* whdr    = calloc(1, sizeof(WAVEHDR));
    HWAVEOUT device  = (HWAVEOUT)audio.device;
    
    whdr->lpData          = (LPSTR)audio.buffer;
    whdr->dwBufferLength  = audio.size * sizeof(int16_t);
    whdr->dwFlags         = 0;

    waveOutPrepareHeader(device, whdr, sizeof(WAVEHDR));
    waveOutWrite(device, whdr, sizeof(WAVEHDR));
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

int16_t float_to_int16(float f)
{
    // clamp to [-1,1] just in case
    if (f > 1.0f)
        f = 1.0f;
    if (f < -1.0f)
        f = -1.0f;

    return (int16_t)(f * 32767.0f);
}

#ifdef PLATFORM_WINDOWS
void CALLBACK audio_event_handler_win(
    HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2) 
{
    switch (uMsg) {
        case WOM_DONE: {
            printf("audio_event_handler(): handling WOM_DONE Event.\n");
            WAVEHDR* pHdr = (WAVEHDR*)dwParam1;
            waveOutUnprepareHeader(h_waveOut, pHdr, sizeof(WAVEHDR));
            free(pHdr);         // free header

            break;
        }
    }
}
#endif