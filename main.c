#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#define OLIVEC_IMPLEMENTATION
#include "deps/olivec/olive.c"

#ifndef M_PI
#   define M_PI 3.14159265
#endif


typedef struct AudioDevice_st AudioDevice;

struct AudioBuffer_st {
    int16_t*     buffer;
    size_t       size;
    AudioDevice* device;
    size_t       time;
    bool         playing;
}; // struct AudioBuffer

typedef struct AudioBuffer_st AudioBuffer;

int16_t      float_to_int16(float f);
AudioBuffer  audio_buffer_create(size_t sample_count, AudioDevice* device);
void         audio_buffer_free(AudioBuffer ab);
void         audio_buffer_update(AudioBuffer* audio);
void         audio_buffer_play(AudioBuffer* audio);
void         audio_buffer_pause(AudioBuffer* audio);
void         audio_buffer_seek_start(AudioBuffer* audio);
void         audio_buffer_seek(AudioBuffer* audio, size_t time);

AudioDevice* audio_init_device();

static AudioDevice* s_audioDevice = NULL;

#define PLATFORM_WINDOWS


#define AUDIO_CHANNELS          2
#define SAMPLE_RATE             44100
#define FPS                     60
#define AUDIO_SAMPLES_PER_FRAME SAMPLE_RATE / FPS 

#define AUDIO_BUFFERS      4
#define AUDIO_BUFFERS_SIZE 2 * AUDIO_SAMPLES_PER_FRAME

static int16_t* s_audioBuffers[AUDIO_BUFFERS];

#ifdef PLATFORM_WINDOWS

static int     s_Audio_CurrentWHDR = 0;
static WAVEHDR s_Audio_WinHDRS[AUDIO_BUFFERS];

#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_WINDOWS
void CALLBACK audio_event_handler_win(HWAVEOUT h_waveOut, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

// fills buffer with sin function samples.
void audio_buffer_sin_fill_mono(AudioBuffer buff);
void audio_buffer_sin_fill_stereo(AudioBuffer buff);

#define CANVAS_WIDTH  600
#define CANVAS_HEIGHT 600

#define GAME_TITLE    "My Little Game"

#ifdef PLATFORM_WINDOWS
#   define GAME_TITLE_CLASS    "MyLittleGameClx"

static BITMAPINFO s_Bmi = {0};

#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Olivec_Canvas canvas;

#define MAIN int main

MAIN() {

    AudioDevice* device = audio_init_device();
    AudioBuffer audio   = audio_buffer_create(SAMPLE_RATE / 2 * 2, NULL);
    audio_buffer_sin_fill_stereo(audio);

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc      = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = GAME_TITLE_CLASS;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) return -1;
  
    HWND hwnd = CreateWindowEx(
        0,
        GAME_TITLE_CLASS,
        GAME_TITLE,
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

    // setup bit map info structure
    s_Bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    s_Bmi.bmiHeader.biWidth       = CANVAS_WIDTH;
    s_Bmi.bmiHeader.biHeight      = -CANVAS_HEIGHT;
    s_Bmi.bmiHeader.biPlanes      = 1;
    s_Bmi.bmiHeader.biBitCount    = 32;
    s_Bmi.bmiHeader.biCompression = BI_RGB;


    HDC hdc     = GetDC(hwnd);
    // mem Device Context
    HDC canvasDC   = CreateCompatibleDC(hdc);

    void* dibPixels;
    HBITMAP hBitmap   = CreateDIBSection(hdc, &s_Bmi, DIB_RGB_COLORS, &dibPixels, NULL, 0);
    HBITMAP oldBitmap = SelectObject(canvasDC, hBitmap);

    canvas = olivec_canvas(
        dibPixels,
        CANVAS_WIDTH, CANVAS_HEIGHT, CANVAS_WIDTH
    );

    MSG msg;
    int frame_count = 0;
    
    HBRUSH winBlackBrush = CreateSolidBrush(RGB(0,0,0)); // black
    RECT clearRect       = {0, 0, CANVAS_WIDTH, CANVAS_HEIGHT};
    
    float x = 10.0f;
    float y = 300.0f;

    float speedX = 400.f;
    float speedY = -400.f;

    while (1) {
        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
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
        
        FillRect(canvasDC, &clearRect, winBlackBrush);
        olivec_rect(canvas, (int)x, (int)y, 40, 40, 0xFFFF0000);

        // memcpy(dibPixels, canvas.pixels, CANVAS_WIDTH * CANVAS_HEIGHT * 4);

        StretchBlt(
            hdc,
            0, 0,
            CANVAS_WIDTH, CANVAS_HEIGHT,
            canvasDC,
            0, 0,
            CANVAS_WIDTH, CANVAS_HEIGHT,
            SRCCOPY
        );

        // BitBlt(
        //     hdc,       // destination DC
        //     0, 0,      // dest x, y
        //     CANVAS_WIDTH, CANVAS_HEIGHT,
        //     canvasDC,     // source DC
        //     0, 0,      // source x, y,
        //     SRCCOPY
        // );

        audio_buffer_update(&audio);

        Sleep(16);
    }


    DeleteObject(winBlackBrush);

    SelectObject(canvasDC, oldBitmap);
    DeleteObject(hBitmap);

    DeleteDC(canvasDC);    
    ReleaseDC(hwnd, hdc);

    getchar();
    printf("finished!\n");

    waveOutClose((HWAVEOUT) device);
    audio_buffer_free(audio);

    return 0;
}

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


void audio_buffer_update(AudioBuffer* audio) {
    if (!audio || !audio->playing) return;

    if (audio->time * 2 >= audio->size) {
        // printf("no more samples to stream.\n");
        return;
    }

    size_t audio_samples = AUDIO_BUFFERS_SIZE;
    if (2*(audio->time + audio_samples) >= audio->size)
        audio_samples = (audio->size/2) - audio->time;

    // Prepare buffer
    WAVEHDR* whdr         = &(s_Audio_WinHDRS[s_Audio_CurrentWHDR]);
    HWAVEOUT device       = (HWAVEOUT)audio->device;

    while (whdr->dwFlags & WHDR_INQUEUE) { Sleep(1); }

    memcpy(whdr->lpData, audio->buffer + 2 * audio->time, audio_samples * 2 * sizeof(int16_t));
    
    whdr->dwBufferLength  = 2 * audio_samples * sizeof(int16_t);

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

#ifdef PLATFORM_WINDOWS

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
#endif // ifdef PLATFORM_WINDOWS

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
            WAVEHDR* pHdr = (WAVEHDR*)dwParam1;

            pHdr->dwFlags &= ~WHDR_INQUEUE;
            break;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif