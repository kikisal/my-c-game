#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define PAR_SHAPES_IMPLEMENTATION
#include "deps/par_shapes/par_shapes.h"    // Shapes 3d parametric generation

#include "game.h"

#define AUDIO_IMPLEMENTATION
#include "audio.h"

#include "deps/glad/glad.h"

#define DEFAULT_ROTMODE ROTMODE_QUATERNION
#define VERTEX_STRIDE 11


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

#define ENGINE_MATH_IMPLEMENTATION
#include "engine_math.h"

typedef struct Window_st * Window;

struct Window_st {
    void* _winHandle;
    char* title;
    int   width;
    int   height;
    int   x;
    int   y;
    bool  focused;
};

static bool     init_platform();
static Window   create_window(size_t width, size_t height, const char* title);
static bool     create_opengl_context(Window window);

GLuint          compile_shader(const char* src, GLenum type);

#define FILE_IMPLEMENTATION
#include "file.h"

#define GLGFX_IMPLEMENTATION
#include "gl_gfx.h"

typedef struct QuadMesh_st  QuadMesh;
typedef struct Camera_st Camera_t;

struct Camera_st {
    vec3 position;
    vec3 target;
    vec3 up;

    vec3 forward;

    float n;
    float f;
    float fov;

    bool needs_update;
    Mat4 matrix;
    Mat4 view_matrix;
    Mat4 proj_matrix;
    Mat4 combined_matrix;
};


struct QuadMesh_st {
    GLuint program;
    GLint  texture_loc;
    GLuint texture;
    GLuint vbo;
    GLuint vao;
};

typedef struct Mesh_st Mesh_t;
typedef struct Transform_st Transform;

struct Transform_st {
    vec3       position;
    vec3       rotation;
    Quaternion rotation_q;
    vec3       scale;

    rot_mode_t rot_mode;
};

// Mesh module

struct Mesh_st {
    GLProgram_t program;
    GLuint      vao;
    GLuint      vbo;
    GLuint      tangent_vbo;

    // memory buffers
    float*      vertices;
    float*      tangents;

    GLuint      ebo;
    size_t      index_count;
    size_t      vertex_count;
    Transform   transform;
    Mat4        localToWorld;
    Texture_t   textures[TEXTURE_COUNT];
    bool        noColorAttrib;
    bool        hasTangentAttrib; 
    bool        showTangentSpace;
    Color       color;
};

void        setQuadMeshProgram(QuadMesh* mesh, GLuint program);
void        renderQuad(QuadMesh mesh);

bool        meshSetupGLBuffers(Mesh_t * mesh, float* vbo_buffer, size_t buff_size);
bool        meshSetupGLBuffers_Raylib(Mesh_t* mesh, float* vertices, float* normals, float* texcoords, float vertex_count);
void        meshInit(Mesh_t* mesh);
QuadMesh    createQuadMesh(size_t texture_width, size_t texture_height, GLuint program);
Mesh_t      createTriangleMesh(vec3 v1, vec3 v2, vec3 v3, Color color, GLProgram_t program);
Mesh_t      createSphereMesh(float radius, int rings, int slices, Color color, GLProgram_t program);
Mesh_t      createCubeMesh(float width, float height, float depth, Color color, GLProgram_t program);
void        renderMesh(Mesh_t m, Camera_t* camera);

Camera_t    camera_init(vec3 position, vec3 target, float near_plane, float far_plane, float fov);
void        update_camera(Camera_t* camera);
void        camera_compute_matrices(Camera_t* camera);
void        camera_compute_viewmatrix(Camera_t* camera);
void        camera_compute_projmatrix(Camera_t* camera);
vec3        screen_to_camera(Camera_t* camera, long screenX, long screenY);

#define LIGHT_IMPLEMENTATION
#include "light.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb_image/stb_image.h"

// io utils

Texture_t loadTextureFromFile(const char * filePath);


// keyboard and mouse
struct KeyState_st {
    uint64_t key;
    bool     down; 
};

typedef struct KeyState_st KeyState;

struct MouseState_st {
    long deltaX;
    long deltaY;
    long x, y;
    long oldX, oldY;    
};

typedef struct MouseState_st MouseState;

void print_mouse_state();

// -- engine constants & state. --
GLuint      g_arrowVBO;
GLuint      g_arrowVAO;
GLProgram_t g_arrowProgram;

#define MOUSE_SENSITIVITY 10.0f

Camera_t camera;

#define KEYMAP_COUNT sizeof(keyState) / sizeof(KeyState)

KeyState keyState[] = {
    {.key = 'W',       .down = false},
    {.key = 'A',       .down = false},
    {.key = 'S',       .down = false},
    {.key = 'D',       .down = false},
    {.key = VK_SPACE,  .down = false},
    {.key = VK_LSHIFT, .down = false},
};

MouseState mouseState;
Window window;

GLint  cameraPosLoc;
Mesh_t cube;
Mesh_t sphere;
Mesh_t floorMesh;

int main() {
    AudioDevice* device = audio_init_device();
    init_platform();

    window = create_window(CANVAS_WIDTH, CANVAS_HEIGHT, GAME_TITLE);
    if (!window) {
        printf("create_window() failed! Quitting.\n");
        return -1;
    }

    if (!create_opengl_context(window)) {
        printf("create_opengl_context() failed! Quitting.\n");
        return -1;
    }

    glGenBuffers(1, &g_arrowVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &g_arrowVAO);
    glBindVertexArray(g_arrowVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    File_t arrow_vs_file = read_file("./shaders/arrow.vs");
    File_t arrow_fs_file = read_file("./shaders/arrow.fs");
    g_arrowProgram = createShaderProgramGL(arrow_vs_file, arrow_fs_file);


    File_t quad_vs_file    = read_file("./shaders/quad.vs");
    File_t quad_fs_file    = read_file("./shaders/quad.fs");
    File_t default_vs_file = read_file("./shaders/default.vs");
    File_t default_fs_file = read_file("./shaders/default.fs");

    if (!quad_vs_file.data || !quad_fs_file.data) {
        if (!quad_vs_file.data) printf("%s was not found.\n", quad_vs_file.file_path);
        if (!quad_fs_file.data) printf("%s was not found.\n", quad_fs_file.file_path);
        return -1;
    }

    if (!default_vs_file.data || !default_fs_file.data) {
        if (!default_vs_file.data) printf("%s was not found.\n", default_vs_file.file_path);
        if (!default_fs_file.data) printf("%s was not found.\n", default_fs_file.file_path);
        return -1;
    }

    Mat4 world = mat4_identity();

    GLuint quadProg         = createShaderProgramGL_(quad_vs_file, quad_fs_file);
    QuadMesh quadMesh       = createQuadMesh(CANVAS_WIDTH, CANVAS_HEIGHT, quadProg);

    GLProgram_t defaultProg = createShaderProgramGL(default_vs_file, default_fs_file);

    QuadMesh quad = createQuadMesh(CANVAS_WIDTH, CANVAS_HEIGHT, quadProg);
    sphere    = createSphereMesh(1.0f, 8, 8, (Color) {1.0f, 1.0f, 1.0f}, defaultProg);
    cube      = createCubeMesh(1.0f, 1.0f, 1.0f, (Color) {1.0f, 1.0f, 1.0f}, defaultProg);
    cube.showTangentSpace = true;

    floorMesh = createCubeMesh(10.0f, 0.1f, 10.0f, (Color) {1.0f, 1.0f, 1.0f}, defaultProg);
    floorMesh.noColorAttrib     = true;
    floorMesh.color             = (Color) {1.0f, 1.0f, 1.0f};

    sphere.transform.position.x = -2.0f;

    Light_t* light1   = createLight((vec3) {1.0f, 1.0f, 1.0f}, (Color) {1.0f, 1.0f, 1.0f},   LIGHT_POINT,       defaultProg);
    light1->intensity = 5.0f;
    updateLight(light1, defaultProg);

    Light_t* light2   = createLight((vec3) {-7.0f, 7.0f, -7.0f}, (Color) {1.0f, 1.0f, 1.0f}, LIGHT_POINT,       defaultProg);
    Light_t* dirLight = createLight((vec3) {4.0f, -5.0f, 4.0f}, (Color) {
        (float)0x87/255.0f, (float)0xCE/255.0f, (float)0xFA/255.0f
    }, LIGHT_DIRECTIONAL, defaultProg);

    light2->intensity = 5.0f;
    dirLight->intensity = .5f;
    updateLight(light2, defaultProg);
    updateLight(dirLight, defaultProg);

    Texture_t diffuseMap                  = loadTextureFromFile("./resources/container.png");
    Texture_t specularMap                 = loadTextureFromFile("./resources/SpecularMap2.png");

    // Texture_t brickDiffuseMap             = loadTextureFromFile("./resources/wall2/wall-4-granite-DIFFUSE.jpg");
    // Texture_t brickNormalMap              = loadTextureFromFile("./resources/wall2/wall-4-granite-NORMAL.jpg");
    // Texture_t brickSpecularMap            = loadTextureFromFile("./resources/wall2/wall-4-granite-SPECULAR.jpg");

    Texture_t brickDiffuseMap             = loadTextureFromFile("./resources/brick_diffuse_map.jpg");
    Texture_t brickNormalMap              = loadTextureFromFile("./resources/brick_normal_map.jpg");

    cube.textures[TEXTURE_ALBEDO_MAP]     = brickDiffuseMap;
    cube.textures[TEXTURE_NORMAL_MAP]     = brickNormalMap;

    light1->pos.z = -1.0f;
    light1->pos.y = 1.0f;

    cameraPosLoc  = glGetUniformLocation(defaultProg.program, "camera_pos");

    glUseProgram(defaultProg.program);
    glUniform3f(cameraPosLoc, camera.position.x, camera.position.y, camera.position.z);    
    glUseProgram(0);

    floorMesh.transform.position.y     = -.6f;

    camera = camera_init(
        vec3_init(-2.0f, 1.0f, 3.0f), 
        vec3_init(0.0f),
        0.1f,
        1000.0f,
        degtorad(60.0f)
    );

    int sleepDelay = (int)((1.0f/(float)FPS)*1000.0f);

    glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
#if defined(_WIN32)
    POINT mouseCenter;

    MSG msg;
    // set cursor to window center
    {
        RECT rect;
        GetClientRect(window->_winHandle, &rect);

        mouseCenter.x = (rect.right - rect.left) / 2;
        mouseCenter.y = (rect.bottom - rect.top) / 2;
        ClientToScreen(window->_winHandle, &mouseCenter);
        SetCursorPos(mouseCenter.x, mouseCenter.y);

        mouseState.x      = mouseCenter.x;
        mouseState.y      = mouseCenter.y;
        mouseState.deltaX = 0;
        mouseState.deltaY = 0;
        mouseState.oldX   = mouseCenter.x;
        mouseState.oldY   = mouseCenter.y;
    }
#endif

    // glEnable(GL_CULL_FACE);

    unsigned int hdrFrameBuffer;
    GLuint hdrColorBuffer;
    unsigned int hdrRbo;

    glGenFramebuffers(1, &hdrFrameBuffer);
    glGenTextures(1, &hdrColorBuffer);
    glGenRenderbuffers(1, &hdrRbo);

    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, CANVAS_WIDTH, CANVAS_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);
    
    glBindRenderbuffer(GL_RENDERBUFFER, hdrRbo);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CANVAS_WIDTH, CANVAS_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrRbo); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    quad.texture  = hdrColorBuffer;
    printf("texture loc: %d\n", quad.texture_loc);
    float time = 0.0f;
    while (true) {
        uint64_t begin = get_time_ns();

    #if defined(_WIN32)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            switch(msg.message) {
                case WM_QUIT: return 0;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        POINT pt;
        if (GetCursorPos(&pt)) {
            // ScreenToClient(window->_winHandle, &pt);
            
            mouseState.deltaX = pt.x - mouseCenter.x;
            mouseState.deltaY = pt.y - mouseCenter.y;
            mouseState.x     += mouseState.deltaX;
            mouseState.y     += mouseState.deltaY;
            
            SetCursorPos(mouseCenter.x, mouseCenter.y);
        }

        if (GetForegroundWindow() != window->_winHandle) {
            window->focused = false;
        } else {
            window->focused = true;
        }

    #endif // defined(_WIN32)
        
        time += .001f;
        light1->pos.x = 4.0f * cosf(time * 2 * M_PI * 5.0f);
        light1->pos.z = 4.0f * sinf(time * 2 * M_PI * 5.0f);
        
        updateLight(light1, defaultProg);
        update_camera(&camera);

        // First pass
            
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFrameBuffer);

        glClearColor((float)0x87/255.0f, (float)0xCE/255.0f, (float)0xFA/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        renderMesh(cube, &camera);
        renderMesh(sphere, &camera);
        renderMesh(floorMesh, &camera);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Second pass
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render into quad
        renderQuad(quad);

        
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
    DestroyWindow(window->_winHandle);
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
    // Get screen dimensions
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int winWidth     = winRect.right - winRect.left;
    int winHeight    = winRect.bottom - winRect.top;

    int winPos_x = (screenWidth - winWidth) / 2;
    int winPos_y = (screenHeight - winHeight) / 2;
  
    HWND hwnd = CreateWindowEx(
        0,
        GAME_TITLE_CLASS,
        title,
        WS_OVERLAPPEDWINDOW,
        winPos_x, winPos_y,
        winWidth, winHeight,
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
    ShowCursor(FALSE);

    // setup bit map info structure
    s_Bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    s_Bmi.bmiHeader.biWidth       = width;
    s_Bmi.bmiHeader.biHeight      = -height;
    s_Bmi.bmiHeader.biPlanes      = 1;
    s_Bmi.bmiHeader.biBitCount    = 32;
    s_Bmi.bmiHeader.biCompression = BI_RGB;

    Window wdw      = malloc(sizeof(struct Window_st));
    if (!wdw) return NULL;
    
    wdw->_winHandle = hwnd;

    wdw->title      = (char*)title;
    wdw->width      = winWidth;
    wdw->height     = winHeight;
    wdw->x          = winPos_x;
    wdw->y          = winPos_y;

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = RIDEV_INPUTSINK;
    rid.hwndTarget  = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    return wdw;
}

static bool init_platform() {
    hInstance        = GetModuleHandle(NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_INPUT:
        {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = (LPBYTE)malloc(dwSize);

            if (lpb && GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    // mouseState.deltaX = raw->data.mouse.lLastX;
                    // mouseState.deltaY = raw->data.mouse.lLastY;
                }
            }
            free(lpb);
            break;
        }

        case WM_KEYDOWN:
        {
            UINT vkCode = (UINT)wParam;

            if (vkCode == VK_SHIFT) {
                UINT scancode = (lParam >> 16) & 0xFF;
                vkCode = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
            }

            for (int i = 0; i < KEYMAP_COUNT; ++i) {
                if (keyState[i].key == vkCode) {
                    keyState[i].down = true;
                    break;
                }
            }
            break;
        }

        case WM_KEYUP: {
            UINT vkCode = (UINT)wParam;

            if (vkCode == VK_SHIFT) {
                UINT scancode = (lParam >> 16) & 0xFF;
                vkCode = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
            }

            for (int i = 0; i < KEYMAP_COUNT; ++i) {
                if (keyState[i].key == vkCode) {
                    keyState[i].down = false;
                    break;
                }
            }
            break;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static bool create_opengl_context(Window window) {
    s_hdc   = GetDC((HWND)window->_winHandle);

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

QuadMesh createQuadMesh(size_t texture_width, size_t texture_height, GLuint program) {
    float quad_vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f
    };

    GLuint quad_vao, quad_vbo;
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint quadTexture;
    glGenTextures(1, &quadTexture);
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    QuadMesh result = (QuadMesh) {
        .program     = program,
        .texture_loc = program > 0 ? glGetUniformLocation(program, "uTexture") : -1,
        .texture     = quadTexture,
        .vbo         = quad_vbo,
        .vao         = quad_vao
    };

    if (program && result.texture_loc > -1) {
        glUseProgram(program);
        glUniform1i(result.texture_loc, 0);
        glUseProgram(0);
    }

    return result;
}

void setQuadMeshProgram(QuadMesh* mesh, GLuint program) {
    if (!mesh) return;
    mesh->program = program;
}

void renderQuad(QuadMesh mesh) {
    if (mesh.program <= 0) return;

    glUseProgram(mesh.program);    
    glBindVertexArray(mesh.vao);

    if (mesh.texture_loc <= -1) {
        mesh.texture_loc = glGetUniformLocation(mesh.program, "uTexture");
        glUniform1i(mesh.texture_loc, 0);
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glBindVertexArray(0);
}

bool meshSetupGLBuffers_Raylib(Mesh_t* mesh, float* vertices, float* normals, float* texcoords, float vertex_count) {
    if (!mesh) return false;
    GLuint vao, vertex_obj, normal_obj, texcoord_obj;

    glGenBuffers(1, &vertex_obj);
    glGenBuffers(1, &normal_obj);
    glGenBuffers(1, &texcoord_obj);
    glGenVertexArrays(1, &vao);
    if (!vao || !vertex_obj || !normal_obj || !texcoord_obj) 
        return false;

    size_t vertex_vbo_size     = vertex_count * 3 * sizeof(float);
    size_t normals_vbo_size    = vertex_count * 3 * sizeof(float);
    size_t texcoords_vbo_size  = vertex_count * 2 * sizeof(float);

    // positions
    glBindBuffer(GL_ARRAY_BUFFER, vertex_obj);
    glBufferData(GL_ARRAY_BUFFER, vertex_vbo_size, vertices, GL_STATIC_DRAW);

    // normals
    glBindBuffer(GL_ARRAY_BUFFER, normal_obj);
    glBufferData(GL_ARRAY_BUFFER, normals_vbo_size, normals, GL_STATIC_DRAW);

    // uvs
    glBindBuffer(GL_ARRAY_BUFFER, texcoord_obj);
    glBufferData(GL_ARRAY_BUFFER, texcoords_vbo_size, texcoords, GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_obj);
    glVertexAttribPointer(ATTRIB_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, normal_obj);
    glVertexAttribPointer(ATTRIB_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, texcoord_obj);
    glVertexAttribPointer(ATTRIB_UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);

    glEnableVertexAttribArray(ATTRIB_POSITION_LOCATION);
    glEnableVertexAttribArray(ATTRIB_NORMAL_LOCATION);
    glEnableVertexAttribArray(ATTRIB_UV_LOCATION);
    glEnableVertexAttribArray(ATTRIB_COLOR_LOCATION);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh->ebo          = 0;
    mesh->index_count  = 0;
    mesh->vao          = vao;
    mesh->vertex_count = vertex_count;
    return true;
}

bool meshSetupGLBuffers(Mesh_t * mesh, float* vbo_buffer, size_t buff_size) {
    if (!mesh) return false;

    GLuint vao, vbo;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    
    if (!vao || !vbo) return false;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buff_size, vbo_buffer, GL_STATIC_DRAW);

    glBindVertexArray(vao);

    // use same buffer for all attributes.
    glVertexAttribPointer(ATTRIB_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) 0);
    glVertexAttribPointer(ATTRIB_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) (3 * sizeof(float)));
    glVertexAttribPointer(ATTRIB_UV_LOCATION, 2, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) (6 * sizeof(float)));
    glVertexAttribPointer(ATTRIB_COLOR_LOCATION, 4, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) (8 * sizeof(float)));

    glEnableVertexAttribArray(ATTRIB_POSITION_LOCATION);
    glEnableVertexAttribArray(ATTRIB_NORMAL_LOCATION);
    glEnableVertexAttribArray(ATTRIB_UV_LOCATION);
    glEnableVertexAttribArray(ATTRIB_COLOR_LOCATION);

    mesh->ebo          = 0;
    mesh->vbo          = vbo;
    mesh->vao          = vao;

    mesh->index_count  = 0;
    mesh->vertex_count = (buff_size / sizeof(float)) / VERTEX_STRIDE;
    glBindVertexArray(0);

    return true;
}

Mesh_t createSphereMesh(float radius, int rings, int slices, Color color, GLProgram_t program)  {
    Mesh_t mesh = { 0 };

    if ((rings >= 3) && (slices >= 3))
    {
        par_shapes_set_epsilon_degenerate_sphere(0.0);
        par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slices, rings);
        par_shapes_scale(sphere, radius, radius, radius);
        // NOTE: Soft normals are computed internally

        float* vertices   = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));
        float* normals    = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));
        float* texcoords  = (float *)malloc(sphere->ntriangles*3*2*sizeof(float));

        int vertexCount   = sphere->ntriangles*3;
        int triangleCount = sphere->ntriangles;

        for (int k = 0; k < vertexCount; k++)
        {
            vertices[k*3 + 0]  = sphere->points[sphere->triangles[k]*3];
            vertices[k*3 + 1]  = sphere->points[sphere->triangles[k]*3 + 1];
            vertices[k*3 + 2]  = sphere->points[sphere->triangles[k]*3 + 2];

            normals[k*3]       = sphere->normals[sphere->triangles[k]*3];
            normals[k*3 + 1]   = sphere->normals[sphere->triangles[k]*3 + 1];
            normals[k*3 + 2]   = sphere->normals[sphere->triangles[k]*3 + 2];

            texcoords[k*2]     = sphere->tcoords[sphere->triangles[k]*2];
            texcoords[k*2 + 1] = sphere->tcoords[sphere->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(sphere);

        // Upload vertex data to GPU (static mesh)
        // UploadMesh(&mesh, false);

        mesh.transform = (Transform) {
            .rot_mode = DEFAULT_ROTMODE,
            .position = {.0f},
            .rotation = {.0f},
            .scale    = {1.0f, 1.0f, 1.0f}
        };

        if (program.program) {
            mesh.program = program;
        }

        if (!meshSetupGLBuffers_Raylib(&mesh, vertices, normals, texcoords, sphere->ntriangles*3))
            return (Mesh_t) {0};

        mesh.noColorAttrib = true;
        meshInit(&mesh);

        mesh.color = color;
        

        free(vertices);
        free(normals);
        free(texcoords);
    }
    else {};

    return mesh;
}

Mesh_t createCubeMesh(float width, float height, float depth, Color color, GLProgram_t prog) {

#   define NORMAL_
#   define UV_
#   define COLOR_
    float vbo_buffer[] = {
        // BACK FACE
        -0.5f, -0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f,  -0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f,   0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
         0.5f,   0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f, -0.5f, NORMAL_ 0.0f, 0.0f, -1.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        
        // FRONT FACE
        -0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
         0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, 0.0f, 1.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,

        // LEFT FACE
        -0.5f,  0.5f,  0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f, -0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f, -0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f, -0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f,  0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f,  0.5f, NORMAL_ -1.0f, 0.0f, 0.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,

        // RIGHT FACE
        0.5f,  0.5f,  0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        0.5f,  0.5f, -0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        0.5f, -0.5f, -0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        0.5f, -0.5f, -0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        0.5f, -0.5f,  0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        0.5f,  0.5f,  0.5f, NORMAL_ 1.0f, 0.0f, 0.0f, UV_  0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        // BOTTOM FACE
        -0.5f, -0.5f, -0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f, -0.5f, -0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
         0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f,  0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f, -0.5f, -0.5f, NORMAL_ 0.0f, -1.0f, 0.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,

        // TOP FACE
        0.5f,   0.5f, -0.5f, NORMAL_ 0.0f, 1.0f, 0.0f,  UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f, -0.5f, NORMAL_ 0.0f, 1.0f, 0.0f, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
         0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 1.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
         0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 1.0f, 0.0f, UV_ 1.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f,  0.5f, NORMAL_ 0.0f, 1.0f, 0.0f, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
        -0.5f,  0.5f, -0.5f, NORMAL_ 0.0f, 1.0f, 0.0f, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
    };


    Mesh_t mesh    = { 0 };
    mesh.transform = (Transform) {
        .rot_mode  = DEFAULT_ROTMODE,
        .position  = {.0f},
        .rotation  = {.0f},
        .scale     = {1.0f, 1.0f, 1.0f}
    };

    mesh.vertices = NULL;
    mesh.tangents = NULL;

    Mat4 localScale       = mat_scale(width, height, depth);
    int vertCount         = (sizeof(vbo_buffer) / sizeof(float)) / VERTEX_STRIDE;
    int tangent_buff_size = 3 * vertCount * sizeof(float);
    float* tangents       = malloc(tangent_buff_size);
    int face_count        = 6;
    int vertex_per_face   = 6;


    mesh.tangents = tangents;
    mesh.vertices = malloc(vertCount * VERTEX_STRIDE * sizeof(float));

    
    // apply scale
    for (int i = 0; i < vertCount; ++i) {
        vbo_buffer[i * VERTEX_STRIDE + 0] *= width;
        vbo_buffer[i * VERTEX_STRIDE + 1] *= height;
        vbo_buffer[i * VERTEX_STRIDE + 2] *= depth;
    }

    memcpy(mesh.vertices, vbo_buffer, vertCount * VERTEX_STRIDE * sizeof(float));

    for (int i = 0; i < face_count; ++i) {
        int index = vertex_per_face * VERTEX_STRIDE * i;
        
        float x1  = vbo_buffer[index + 0];
        float y1  = vbo_buffer[index + 1];
        float z1  = vbo_buffer[index + 2];

        float x2  = vbo_buffer[index + VERTEX_STRIDE + 0];
        float y2  = vbo_buffer[index + VERTEX_STRIDE + 1];
        float z2  = vbo_buffer[index + VERTEX_STRIDE + 2];

        vec3 tangent = vec3_norm((vec3) {
            x2 - x1,
            y2 - y1,
            z2 - z1,
        });
        
        for (int j = 0; j < vertex_per_face; ++j) {
            tangents[i * vertex_per_face * 3 + j * 3 + 0] = tangent.x;
            tangents[i * vertex_per_face * 3 + j * 3 + 1] = tangent.y;
            tangents[i * vertex_per_face * 3 + j * 3 + 2] = tangent.z;
        }
    }

    if (prog.program) {
        mesh.program = prog;
    }

    // TODO: Remove memory leak for tangents and other critical resources.
    if (!meshSetupGLBuffers(&mesh, vbo_buffer, sizeof(vbo_buffer))) 
        return (Mesh_t) {0};

    // upload tangent vectors.
    glGenBuffers(1, &mesh.tangent_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.tangent_vbo);
    glBufferData(GL_ARRAY_BUFFER, tangent_buff_size, tangents, GL_STATIC_DRAW);
    glBindVertexArray(mesh.vao);
    glVertexAttribPointer(ATTRIB_TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(ATTRIB_TANGENT_LOCATION);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    mesh.hasTangentAttrib = true;
    
    meshInit(&mesh);

    return mesh;

#   undef NORMAL_
#   undef UV_
#   undef COLOR_
}

void meshInit(Mesh_t* mesh) {
    if (!mesh) return;

    // if (mesh->program.program) {
    //     for (int i = 0; i < TEXTURE_COUNT; ++i) {
    //         mesh->textures[i]             = (Texture_t) {0};
    //         mesh->uniform_texture_locs[i] = glGetUniformLocation(mesh->program.program, UNIFORM_TEXTURE_NAMES[i]);
    //     }
    // }
} 

Mesh_t createTriangleMesh(vec3 v1, vec3 v2, vec3 v3, Color color, GLProgram_t prog) {
    Mesh_t mesh;

    mesh.transform = (Transform) {
        .rot_mode = DEFAULT_ROTMODE,
        .position = {.0f},
        .rotation = {.0f},
        .scale    = {1.0f, 1.0f, 1.0f}
    };

    if (prog.program) {
        mesh.program = prog;
    }

    // compute normal vector
    vec3 side1  = vec3_sub(v1, v2);
    vec3 side2  = vec3_sub(v1, v3);
    vec3 normal = vec3_norm(vec3_cross(side1, side2));

#   define NORMAL_
#   define UV_
#   define COLOR_

    float vbo_buffer[] = {
        v1.x, v1.y, v1.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        v2.x, v2.y, v2.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        v3.x, v3.y, v3.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
    };

    if (!meshSetupGLBuffers(&mesh, vbo_buffer, sizeof(vbo_buffer))) 
        return (Mesh_t) {0};
    

    meshInit(&mesh);

    return mesh;

#   undef NORMAL_
#   undef UV_
}

void drawArrow(vec3 position, vec3 dir, Color color) {            
    glUseProgram(g_arrowProgram.program);
    glUniform3f(g_arrowProgram.color_loc, color.r, color.g, color.b);
    
    vec3 lineEnd = vec3_add(position, vec3_scale(dir, .1f));

    float data[] = {
        position.x, position.y, position.z,
        lineEnd.x,      lineEnd.y,      lineEnd.z,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, g_arrowVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * 3 * sizeof(float), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(g_arrowVAO);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    
    glUseProgram(0);
}

void renderMesh(Mesh_t m, Camera_t* camera) {
    if (!m.program.program || !camera) return;

    Mat4 world_mat = mat4_identity();
    
    if (m.transform.rot_mode == ROTMODE_QUATERNION) {
        world_mat = quat_to_mat4(m.transform.rotation_q);
    } else {
        // hope and pray for no Gimbal lock
        world_mat = mat_rotate_x(m.transform.rotation.x);
        world_mat = mat_mul(mat_rotate_y(m.transform.rotation.y), world_mat);
        world_mat = mat_mul(mat_rotate_z(m.transform.rotation.z), world_mat);
    }

    world_mat = mat_mul(mat_scale(m.transform.scale.x, m.transform.scale.y, m.transform.scale.z), world_mat);
    world_mat = mat_mul(mat_translate(m.transform.position.x, m.transform.position.y, m.transform.position.z), world_mat);
    // M = T * S * R 
    m.localToWorld = world_mat; 
        
    camera_compute_matrices(camera);

    glBindVertexArray(m.vao);
    glUseProgram(m.program.program);

    glUniformMatrix4fv(m.program.view_mat_loc,  1, GL_TRUE, (float*)(&camera->view_matrix));
    glUniformMatrix4fv(m.program.proj_mat_loc,  1, GL_TRUE, (float*)(&camera->proj_matrix));
    glUniformMatrix4fv(m.program.world_mat_loc, 1, GL_TRUE, (float*)(&m.localToWorld));
    glUniform3f(cameraPosLoc, camera->position.x, camera->position.y, camera->position.z);

    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        if (!m.textures[i].texture_id) continue;

        GLuint tId = m.textures[i].texture_id;

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tId);
    }

    glUniform1i(m.program.no_color_attrib_loc, m.noColorAttrib);
    glUniform3f(m.program.color_loc, m.color.r, m.color.g, m.color.b);
    glUniform1i(m.program.has_tangent_attrib_loc, m.hasTangentAttrib);
    
    if (m.ebo) {
        glDrawElements(GL_TRIANGLES, m.index_count, GL_UNSIGNED_SHORT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m.vertex_count);
    }


    // clean up
    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (m.showTangentSpace) {

        glUseProgram(g_arrowProgram.program);
        glUniformMatrix4fv(g_arrowProgram.view_mat_loc,  1, GL_TRUE, (float*)(&camera->view_matrix));
        glUniformMatrix4fv(g_arrowProgram.proj_mat_loc,  1, GL_TRUE, (float*)(&camera->proj_matrix));
        glUniformMatrix4fv(g_arrowProgram.world_mat_loc, 1, GL_TRUE, (float*)(&m.localToWorld));
        glUseProgram(0);

        for (int i = 0; i < m.vertex_count; ++i) {
            float* vertex         = &(m.vertices[i * VERTEX_STRIDE]);
            float* tangent_vertex = &(m.tangents[3 * i]);

            vec3 position   = vec3_init(vertex[0], vertex[1], vertex[2]);
            vec3 normal     = vec3_init(vertex[3 + 0], vertex[3 + 1], vertex[3 + 2]);
            vec3 tangent    = vec3_init(tangent_vertex[0], tangent_vertex[1], tangent_vertex[2]);

            // // re-orthogonalization
            tangent = vec3_norm(vec3_sub(tangent, vec3_scale(normal, vec3_dot(tangent, normal))));
            vec3 bitangent = vec3_cross(normal, tangent);
            
            drawArrow(position, tangent,   (Color) {1.0f, 0.0f, 0.0});
            drawArrow(position, bitangent, (Color) {0.0f, 1.0f, 0.0});
            drawArrow(position, normal,    (Color) {0.0f, 0.0f, 1.0});
        }   
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void camera_compute_matrices(Camera_t* camera) {
    if (!camera) return;
    
    camera_compute_viewmatrix(camera);
    camera_compute_projmatrix(camera);
    camera->combined_matrix = mat_mul(camera->proj_matrix, camera->view_matrix);
    camera->needs_update = false;
}

// TODO: Implement FOV parameter.
Camera_t camera_init(vec3 position, vec3 target, float near_plane, float far_plane, float fov) {
    return (Camera_t) {
        .position = position,
        .target   = target,
        .up       = vec3_init(0.0f, 1.0f, 0.0f),
        .n        = near_plane,
        .f        = far_plane,
        .fov      = fov
    };
}

void update_camera(Camera_t * camera) {
    if (!camera) return;

    float cameraSpeed = .05f;

    vec3 forwardDelta = screen_to_camera(camera, mouseState.deltaX + CANVAS_WIDTH / 2, mouseState.deltaY + CANVAS_HEIGHT / 2);
    forwardDelta.x = forwardDelta.x * MOUSE_SENSITIVITY;
    forwardDelta.y = forwardDelta.y * MOUSE_SENSITIVITY;

    camera->forward.x += forwardDelta.x;
    camera->forward.y += forwardDelta.y;
    camera->forward    = vec3_norm(camera->forward);

    camera->target = mat_transform(camera->forward, camera->matrix, 1.0f);
    // reset camera forward vector
    camera->forward = (vec3) {0.0f, 0.0f, -1.0f};

    camera_compute_viewmatrix(camera);

    for (int i = 0; i < KEYMAP_COUNT; ++i) {
        if (!keyState[i].down) continue;

        vec3 forward = {
            .x = -camera->matrix.m02, 
            .y = -camera->matrix.m12, 
            .z = -camera->matrix.m22,
        };

        vec3 right = {
            .x = camera->matrix.m00, 
            .y = camera->matrix.m10, 
            .z = camera->matrix.m20,
        };

        vec3 up = {
            .x  = camera->matrix.m01, 
            .y  = camera->matrix.m11, 
            .z  = camera->matrix.m21,
        };

        switch (keyState[i].key) {
            case 'W': {
                camera->position.x += forward.x * cameraSpeed;
                camera->position.y += forward.y * cameraSpeed;
                camera->position.z += forward.z * cameraSpeed;
                break;
            }                
            case 'S': {
                camera->position.x += -forward.x * cameraSpeed;
                camera->position.y += -forward.y * cameraSpeed;
                camera->position.z += -forward.z * cameraSpeed;
                break;
            }
            case 'A': {
                camera->position.x += -right.x * cameraSpeed;
                camera->position.y += -right.y * cameraSpeed;
                camera->position.z += -right.z * cameraSpeed;
                break;
            }
            case 'D':
            {
                camera->position.x += right.x * cameraSpeed;
                camera->position.y += right.y * cameraSpeed;
                camera->position.z += right.z * cameraSpeed;
                break;
            }
            case VK_SPACE: {
                camera->position.x += up.x * cameraSpeed;
                camera->position.y += up.y * cameraSpeed;
                camera->position.z += up.z * cameraSpeed;
                break;
            }
            case VK_LSHIFT: {
                camera->position.x += -up.x * cameraSpeed;
                camera->position.y += -up.y * cameraSpeed;
                camera->position.z += -up.z * cameraSpeed;
                break;
            }
        }

        camera->target = vec3_add(camera->position, forward);
    }
}

void camera_compute_viewmatrix(Camera_t* camera) {
    vec3 z_axis         = vec3_norm(vec3_sub(camera->position, camera->target));
    vec3 x_axis         = vec3_norm(vec3_cross(camera->up, z_axis));
    vec3 y_axis         = vec3_norm(vec3_cross(z_axis, x_axis));
    
    camera->matrix      = mat_translate(camera->position.x, camera->position.y, camera->position.z);
    camera->matrix.m00  = x_axis.x;
    camera->matrix.m10  = x_axis.y;
    camera->matrix.m20  = x_axis.z;

    camera->matrix.m01  = y_axis.x;
    camera->matrix.m11  = y_axis.y;
    camera->matrix.m21  = y_axis.z;

    camera->matrix.m02  = z_axis.x;
    camera->matrix.m12  = z_axis.y;
    camera->matrix.m22  = z_axis.z;
    
    camera->view_matrix = mat_inv(camera->matrix);
}

vec3 screen_to_camera(Camera_t* camera, long screenX, long screenY) {
    float sX = screenX;
    float sY = screenY;

    // TODO: Handle dynamic screen sizes.
    float ndcX = 2.0f*(screenX / (float)CANVAS_WIDTH) - 1.0f;
    float ndcY = 1.0f - 2.0f*(screenY / (float)CANVAS_HEIGHT);

    float w = camera->n;
    vec3 clip = {.x = ndcX * w, .y = ndcY * w, .z = 0.0f};
    clip.z = w * (camera->f + camera->n) / (camera->f - camera->n) + (2.0f * camera->f * camera->n) / (camera->f - camera->n);
    Mat4 clip_to_camera = mat_inv(camera->proj_matrix);

    return mat_transform(clip, clip_to_camera, w);
}

void camera_compute_projmatrix(Camera_t* camera) {
    // TODO: Handle dynamic window size changes.
    float aspect = (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT;
    
    float height =  2.0f * camera->n * tanf(camera->fov / 2.0f);
    float width  = height * aspect;
    
    float left   = -width  * .5f;
    float right  =  width  * .5f;
    float top    =  height * .5f;
    float bottom = -height * .5f;

    camera->proj_matrix     = mat4_identity();
    camera->proj_matrix.m00 = 2.0f * camera->n / width;
    camera->proj_matrix.m11 = 2.0f * camera->n / height;
    camera->proj_matrix.m22 = -(camera->f + camera->n)/(camera->f - camera->n);
    camera->proj_matrix.m23 = (-2.0f * camera->f * camera->n) / (camera->f - camera->n);
    
    camera->proj_matrix.m02 = (right + left) / 1.0f;
    camera->proj_matrix.m12 = (top + bottom) / height;
    
    camera->proj_matrix.m32 = -1.0f;
    camera->proj_matrix.m33 = 0.0f;
}

void print_mouse_state()
{
    printf("mouse X: %d, Y: %d, dX: %d, dY: %d\n", mouseState.x, mouseState.y, mouseState.deltaX, mouseState.deltaY);
}

Texture_t loadTextureFromFile(const char * filePath) {
    Texture_t texture;
    texture.texture_id = 0;

    int width, height, channels;
    unsigned char *data = stbi_load(filePath, &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load texture: %s\n", filePath);
        return texture;
    }

    glGenTextures(1, &texture.texture_id);
    glBindTexture(GL_TEXTURE_2D, texture.texture_id);

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return texture;
}