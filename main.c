#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#include "game.h"

#define AUDIO_IMPLEMENTATION
#include "audio.h"

#include "deps/glad/glad.h"


#if !defined(_WIN32)
    #include <time.h>
#endif // !defined(_WIN32)

#if defined(_WIN32)
#   include <GL/wglext.h>
#endif // defined(_WIN32)

uint64_t get_time_ns();

#if defined(_WIN32)

#define GAME_TITLE_CLASS    "MyLittleGameClx"

static BITMAPINFO s_Bmi = {0};
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HGLRC s_hglrc;
static HDC   s_hdc;
static HINSTANCE hInstance;

#endif // defined(_WIN32)

// @deprecated
#define OLIVEC_IMPLEMENTATION
#include "deps/olivec/olive.c"

// mat lib
typedef struct Mat4_st Mat4;

struct Mat4_st {
    float m00, m01, m02, m03,
          m10, m11, m12, m13,
          m20, m21, m22, m23,
          m30, m31, m32, m33;
};

#define MATDEF static

MATDEF Mat4 mat_mul(Mat4 m1, Mat4 m2);
MATDEF Mat4 mat4_identity();
MATDEF Mat4 mat_translate(float x, float y, float z);
MATDEF Mat4 mat_rotate_x(float angle);
MATDEF Mat4 mat_rotate_y(float angle);
MATDEF Mat4 mat_rotate_z(float angle);

typedef struct Window_st * Window;

static bool     init_platform();
static Window   create_window(size_t width, size_t height, const char* title);
static bool     create_opengl_context(Window window);

GLuint          compile_shader(const char* src, GLenum type);


static Olivec_Canvas canvas;

// simple shader sources
const char* vs_src =
    "#version 440 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUv;\n"
    "out vec2 uvOut;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    uvOut = aUv;\n"
    "}\n";

const char* fs_src =
    "#version 440 core\n"
    "out vec4 FragColor;\n"
    "in vec2 uvOut;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "    FragColor = vec4(texture(uTexture, uvOut).rgb, 1.0);\n"
    "}\n";
    
// M = T * R * S

int main() {
    AudioDevice* device = audio_init_device();
    
    Mat4 world = mat4_identity();

    init_platform();

    Window window = create_window(CANVAS_WIDTH, CANVAS_HEIGHT, GAME_TITLE);
    
    if (!create_opengl_context(window)) {
        printf("create_opengl_context() failed! Quitting.\n");
        return -1;
    }

    GLuint vs = compile_shader(vs_src, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint worldMatLocation = glGetUniformLocation(prog, "world");
    GLint quadTextureLoc   = glGetUniformLocation(prog, "uTexture");
    
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    canvas = olivec_canvas(
        malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(uint32_t)),
        CANVAS_WIDTH, CANVAS_HEIGHT, CANVAS_WIDTH
    );

    game_init(canvas);
    int sleepDelay = (int)((1.0f/(float)FPS)*1000.0f);

    glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
#if defined(_WIN32)
    MSG msg;
#endif

    GLuint quadTexture;
    glGenTextures(1, &quadTexture);
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CANVAS_WIDTH, CANVAS_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, canvas.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    float angle = 0.0f;
    
    glEnable(GL_DEPTH);

    while (true) {
        uint64_t begin = get_time_ns();

    #if defined(_WIN32)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    #endif // defined(_WIN32)
        
        olivec_fill(canvas, 0x00);
        game_update(canvas);

        angle += .1f;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);
        
        glBindVertexArray(vao);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, quadTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CANVAS_WIDTH, CANVAS_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, canvas.pixels);
        glUniform1i(quadTextureLoc, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        
        SwapBuffers(s_hdc);
        
        uint64_t end = get_time_ns();

        uint64_t ellapsed = (end - begin) / 1000000LL;
        
        if (ellapsed < 16) {
        #if defined(_WIN32)
            Sleep(sleepDelay - ellapsed);
        #endif // defined(_WIN32)
        }
    }

    // destroy game
    game_close();
    
#if defined(_WIN32)
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(s_hglrc);

    ReleaseDC((HWND) window, s_hdc);
    waveOutClose((HWAVEOUT) device);
#endif // defined(_WIN32)
    return 0;
}

#if defined(_WIN32)

static Window create_window(size_t width, size_t height, const char* title) {

    WNDCLASS wc      = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = GAME_TITLE_CLASS;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) return NULL;

    RECT winRect = { 0, 0, width, height };
    AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE);
  
    HWND hwnd = CreateWindowEx(
        0,
        GAME_TITLE_CLASS,
        title,
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

        return NULL;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);

    // setup bit map info structure
    s_Bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    s_Bmi.bmiHeader.biWidth       = width;
    s_Bmi.bmiHeader.biHeight      = -height;
    s_Bmi.bmiHeader.biPlanes      = 1;
    s_Bmi.bmiHeader.biBitCount    = 32;
    s_Bmi.bmiHeader.biCompression = BI_RGB;

    return (Window)hwnd;
}

static bool init_platform() {
    hInstance        = GetModuleHandle(NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static bool create_opengl_context(Window window) {
    s_hdc   = GetDC((HWND)window);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24, // depth buffer
        8,  // stencil buffer
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(s_hdc, &pfd);
    SetPixelFormat(s_hdc, pixelFormat, &pfd);

    HGLRC tmpCtx = wglCreateContext(s_hdc);
    wglMakeCurrent(s_hdc, tmpCtx);

    // load the extension function
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

    int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, OPENGL_MAJOR_VERSION,
        WGL_CONTEXT_MINOR_VERSION_ARB, OPENGL_MINOR_VERSION,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    s_hglrc = wglCreateContextAttribsARB(s_hdc, 0, attribs);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tmpCtx);

    wglMakeCurrent(s_hdc, s_hglrc);

    if (!gladLoadGL()) {
        MessageBox((HWND)window, "Failed to load OpenGL functions!", "Error", MB_OK);
        return false;
    }

    return true;
}
#else
#error "create_opengl_context()  not implemented in your current OS."
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


GLuint compile_shader(const char* src, GLenum type) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    int success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(sh, 512, NULL, log);
        MessageBox(NULL, log, "Shader compile error", MB_OK);
    }
    return sh;
}

// mat module

MATDEF Mat4 mat_mul(Mat4 m1, Mat4 m2) {
    Mat4 result = {0.0f};
    result.m00 = m1.m00 * m2.m00 + m1.m01 * m2.m10 + m1.m02 * m2.m20 + m1.m03 * m2.m30;
    result.m01 = m1.m00 * m2.m01 + m1.m01 * m2.m11 + m1.m02 * m2.m21 + m1.m03 * m2.m31;
    result.m02 = m1.m00 * m2.m02 + m1.m01 * m2.m12 + m1.m02 * m2.m22 + m1.m03 * m2.m32;
    result.m03 = m1.m00 * m2.m03 + m1.m01 * m2.m13 + m1.m02 * m2.m23 + m1.m03 * m2.m33;

    result.m10 = m1.m10 * m2.m00 + m1.m11 * m2.m10 + m1.m12 * m2.m20 + m1.m13 * m2.m30;
    result.m11 = m1.m10 * m2.m01 + m1.m11 * m2.m11 + m1.m12 * m2.m21 + m1.m13 * m2.m31;
    result.m12 = m1.m10 * m2.m02 + m1.m11 * m2.m12 + m1.m12 * m2.m22 + m1.m13 * m2.m32;
    result.m13 = m1.m10 * m2.m03 + m1.m11 * m2.m13 + m1.m12 * m2.m23 + m1.m13 * m2.m33;

    result.m20 = m1.m20 * m2.m00 + m1.m21 * m2.m10 + m1.m22 * m2.m20 + m1.m23 * m2.m30;
    result.m21 = m1.m20 * m2.m01 + m1.m21 * m2.m11 + m1.m22 * m2.m21 + m1.m23 * m2.m31;
    result.m22 = m1.m20 * m2.m02 + m1.m21 * m2.m12 + m1.m22 * m2.m22 + m1.m23 * m2.m32;
    result.m23 = m1.m20 * m2.m03 + m1.m21 * m2.m13 + m1.m22 * m2.m23 + m1.m23 * m2.m33;
    
    result.m30 = m1.m30 * m2.m00 + m1.m31 * m2.m10 + m1.m32 * m2.m20 + m1.m33 * m2.m30;
    result.m31 = m1.m30 * m2.m01 + m1.m31 * m2.m11 + m1.m32 * m2.m21 + m1.m33 * m2.m31;
    result.m32 = m1.m30 * m2.m02 + m1.m31 * m2.m12 + m1.m32 * m2.m22 + m1.m33 * m2.m32;
    result.m33 = m1.m30 * m2.m03 + m1.m31 * m2.m13 + m1.m32 * m2.m23 + m1.m33 * m2.m33;

    return result;
}

MATDEF Mat4 mat4_identity() {
    Mat4 result = {0.0f};
    result.m00 = 1.0f;
    result.m11 = 1.0f;
    result.m22 = 1.0f;
    result.m33 = 1.0f;
    return result;
}

MATDEF Mat4 mat_translate(float x, float y, float z) {
    Mat4 result = mat4_identity();
    
    result.m03 = x;
    result.m13 = y;
    result.m23 = z;
    return result;
}

MATDEF Mat4 mat_rotate_x(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    result.m11 = c;
    result.m12 = -s;
    result.m21 = s;
    result.m22 = c;
    return result;
}

MATDEF Mat4 mat_rotate_y(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    result.m00 = c;
    result.m02 = s;
    result.m20 = -s;
    result.m22 = c;

    return result;
}

MATDEF Mat4 mat_rotate_z(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    result.m00 = c;
    result.m01 = -s;
    result.m10 = s;
    result.m11 = c;

    return result;
}
