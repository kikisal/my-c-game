#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "game.h"

#define PLATFORM_WINDOWS
#define AUDIO_IMPLEMENTATION
#include "audio.h"

#define OLIVEC_IMPLEMENTATION
#include "deps/olivec/olive.c"

#define GAME_TITLE    "My Little Game"

#ifdef PLATFORM_WINDOWS
#   define GAME_TITLE_CLASS    "MyLittleGameClx"

static BITMAPINFO s_Bmi = {0};

#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Olivec_Canvas canvas;

#define MAIN int main

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <time.h>
#endif

// Returns current time in nanoseconds (monotonic, high-resolution)
uint64_t get_time_ns() {
#if defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }

    QueryPerformanceCounter(&counter);
    return (uint64_t)( (counter.QuadPart * 1000000000ULL) / frequency.QuadPart );
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

MAIN() {

    AudioDevice* device = audio_init_device();

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc      = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = GAME_TITLE_CLASS;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) return -1;

    RECT winRect = { 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT };
    AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE);
  
    HWND hwnd = CreateWindowEx(
        0,
        GAME_TITLE_CLASS,
        GAME_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winRect.right - winRect.left,
        winRect.bottom - winRect.top,
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
    
    game_init(canvas);
    

    int sleepDelay = (int)((1.0f/(float)FPS)*1000.0f);

    while (1) {

        uint64_t begin = get_time_ns();
 
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        FillRect(canvasDC, &clearRect, winBlackBrush);
        
        // olivec_fill(canvas, 0x00);
        game_update(canvas);
        
        // memcpy(dibPixels, canvas.pixels, canvas.width * canvas.height * sizeof(uint32_t));
        
        BitBlt(
            hdc,
            0, 0,
            CANVAS_WIDTH, CANVAS_HEIGHT,
            canvasDC,
            0, 0,
            SRCCOPY
        );

        uint64_t end = get_time_ns();

        uint64_t ellapsed = (end - begin) / 1000000LL;
        
        // printf("%lld\n", ellapsed);
        if (ellapsed < 16)
            Sleep(sleepDelay - ellapsed);
    }

    // destroy game
    game_close();

    DeleteObject(winBlackBrush);

    SelectObject(canvasDC, oldBitmap);
    DeleteObject(hBitmap);

    DeleteDC(canvasDC);
    ReleaseDC(hwnd, hdc);

    getchar();
    printf("finished!\n");

    waveOutClose((HWAVEOUT) device);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}