#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#define OLIVEC_IMPLEMENTATION
#include "deps/olivec/olive.c"

#ifndef M_PI
#   define M_PI 3.14159265
#endif

#define SAMPLE_RATE 44100

typedef struct AudioDevice_st AudioDevice;

struct AudioBuffer_st {
    int16_t*     buffer;
    size_t       size;
    AudioDevice* device;
}; // struct AudioBuffer

typedef struct AudioBuffer_st AudioBuffer;

int16_t     float_to_int16(float f);
AudioBuffer audio_buffer_create(size_t sample_count, AudioDevice* device);
void        audio_buffer_free(AudioBuffer ab);
void        audio_buffer_play(AudioBuffer audio);

#define PLATFORM_WINDOWS

#ifdef PLATFORM_WINDOWS
void CALLBACK audio_event_handler_win(HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

// fills buffer with sin function samples.
void audio_buffer_sin_fill_mono(AudioBuffer buff);
void audio_buffer_sin_fill_stereo(AudioBuffer buff);

#define CANVAS_WIDTH  800
#define CANVAS_HEIGHT 600
#define GAME_TITLE    "My Little Game"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Olivec_Canvas canvas;


int main() {

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc      = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = GAME_TITLE;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) return -1;

    // Adjust window size to fit client area
  
    HWND hwnd = CreateWindowEx(
        0,
        GAME_TITLE,
        "Canvas Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CANVAS_WIDTH,
        CANVAS_HEIGHT,
        NULL, NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        DWORD err = GetLastError();
        char buf[256];
        sprintf(buf, "CreateWindowEx failed with error %lu", err);
        MessageBox(NULL, buf, "Error", MB_ICONERROR);

        return -1;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    
    canvas = olivec_canvas(
        malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(uint32_t)), 
        CANVAS_WIDTH, CANVAS_HEIGHT, CANVAS_WIDTH
    );

    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        olivec_rect(canvas, 0, 0, 300, 300, 0xFF00FFFF);

        // Update canvas here...
        InvalidateRect(hwnd, NULL, FALSE);
        Sleep(16);
    }



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


    // displayPixels();

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            BITMAPINFO bmi;
            ZeroMemory(&bmi, sizeof(BITMAPINFO));
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = CANVAS_WIDTH;
            bmi.bmiHeader.biHeight      = -CANVAS_HEIGHT;
            bmi.bmiHeader.biPlanes      = 1;
            bmi.bmiHeader.biBitCount    = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            StretchDIBits(
                hdc,
                0, 0, CANVAS_WIDTH, CANVAS_HEIGHT,
                0, 0, CANVAS_WIDTH, CANVAS_HEIGHT,
                canvas.pixels,
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );

            EndPaint(hwnd, &ps);
        } return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif