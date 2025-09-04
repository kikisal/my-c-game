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

#define DEFAULT_ROTMODE ROTMODE_QUATERNION

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

#define EPS 0.00001f

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
MATDEF Mat4 mat_scale(float x, float y, float z);
MATDEF Mat4 mat_rotate_x(float angle);
MATDEF Mat4 mat_rotate_y(float angle);
MATDEF Mat4 mat_rotate_z(float angle);

typedef struct Vec3_st {
    float x, y, z;
} Vec3;


Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_norm(Vec3 v);
Vec3 vec3_cross(Vec3 side1, Vec3 side2);

typedef struct Quaternion_st {
    float x, y, z, w;
} Quaternion;

Quaternion quat_add(Quaternion a, Quaternion b);
Quaternion quat_sub(Quaternion a, Quaternion b);
Quaternion quat_mul(Quaternion a, Quaternion b);
Quaternion quat_normalize(Quaternion q);
Quaternion quat_from_axis_angle(Vec3 axis, float angle);
Quaternion quat_rotate(Vec3 direction, float angle);
Mat4       quat_to_mat4(Quaternion q);
Quaternion quat_inverse(Quaternion q);

typedef enum rot_mode_enum {
    ROTMODE_EULER,
    ROTMODE_QUATERNION
} rot_mode_t;

typedef struct Color_st {
    float r, g, b, a;
} Color;

typedef struct Window_st * Window;

static bool     init_platform();
static Window   create_window(size_t width, size_t height, const char* title);
static bool     create_opengl_context(Window window);

GLuint          compile_shader(const char* src, GLenum type);

static Olivec_Canvas canvas;

// simple shader sources


typedef struct GLProgram_st GLProgram_t;
typedef struct File_st      File_t;
typedef struct QuadMesh_st  QuadMesh;
typedef struct Camera_st Camera_t;

struct Camera_st {
    Mat4 view_matrix;
    Mat4 proj_matrix;
    Mat4 combined_matrix;
};

struct File_st {
    char*       data;
    size_t      size;
    const char* file_path;
};

File_t      read_file(const char* filepath);

struct GLProgram_st {
    GLuint program;
    GLint world_mat_loc;
    GLint view_mat_loc;
    GLint proj_mat_loc;
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
    Vec3       position;
    Vec3       rotation;
    Quaternion rotation_q;
    Vec3       scale;

    rot_mode_t rot_mode;
};

struct Mesh_st {
    GLProgram_t program;
    GLuint      vao;
    GLuint      vbo;
    GLuint      ebo;
    size_t      index_count;
    size_t      vertex_count;
    Transform   transform;
    Mat4        localToWorld;
};

void        canvas_to_GLtexture(Olivec_Canvas src, GLint dest);
void        setQuadMeshProgram(QuadMesh* mesh, GLuint program);
void        renderQuad(QuadMesh mesh);
GLuint      createShaderProgramGL_(File_t vs_file, File_t fs_file);
GLProgram_t createShaderProgramGL(File_t vs_file, File_t fs_file);

QuadMesh    createQuadMesh(size_t texture_width, size_t texture_height, GLuint program);
Mesh_t      createTriangleMesh(Vec3 v1, Vec3 v2, Vec3 v3, Color color, GLProgram_t program);
void        renderMesh(Mesh_t m, Camera_t camera);

// -- engine constants. --

#define ATTRIB_POSITION_LOCATION    0
#define ATTRIB_NORMAL_LOCATION      1
#define ATTRIB_UV_LOCATION          2
#define ATTRIB_COLOR_LOCATION       3

#define UNIFORM_WORLD_MATRIX "world_mat"
#define UNIFORM_VIEW_MATRIX  "view_mat"
#define UNIFORM_PROJ_MATRIX  "proj_mat"

Mat4 view_matrix;
Mat4 proj_matrix;

int main() {
    AudioDevice* device = audio_init_device();
    init_platform();

    Window window = create_window(CANVAS_WIDTH, CANVAS_HEIGHT, GAME_TITLE);
    
    if (!create_opengl_context(window)) {
        printf("create_opengl_context() failed! Quitting.\n");
        return -1;
    }

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

    // for now.
    view_matrix = mat4_identity();
    proj_matrix = mat4_identity();
    
    Mat4 world = mat4_identity();

    GLuint quadProg         = createShaderProgramGL_(quad_vs_file, quad_fs_file);
    QuadMesh quadMesh       = createQuadMesh(CANVAS_WIDTH, CANVAS_HEIGHT, quadProg);

    GLProgram_t defaultProg = createShaderProgramGL(default_vs_file, default_fs_file);
    Mesh_t triangle = createTriangleMesh(
        (Vec3) {.5f, 0.0f, 0.0f},
        (Vec3) {0.0f, 0.5f, 0.0f},
        (Vec3) {-.5f, 0.0f, 0.0f},
        (Color) {1.0f, 0.0f, 0.0f, 1.0f}, 
        defaultProg
    );

    Camera_t camera = {
        .view_matrix     = view_matrix,
        .proj_matrix     = proj_matrix,
        .combined_matrix = mat_mul(proj_matrix, view_matrix)
    };

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
        
        // olivec_fill(canvas, 0x00);
        // game_update(canvas);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        angle += .01f;

        triangle.transform.rotation_q = quat_from_axis_angle((Vec3) {.0, 1.0f, 0.0}, angle);
        renderMesh(triangle, camera);
        
        // canvas_to_GLtexture(canvas, quadMesh.texture);
        // renderQuad(quadMesh);
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

MATDEF Mat4 mat_scale(float x, float y, float z) {
    Mat4 result = mat4_identity();
    result.m00 = x;
    result.m11 = y;
    result.m22 = z;
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

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

Vec3 vec3_norm(Vec3 v) {
    float len = v.x*v.x + v.y*v.y + v.z*v.z;
    if (fabsf(len) < EPS) return (Vec3) {0.0f};
    
    len = sqrtf(len);

    return (Vec3) {
        .x = v.x / len,
        .y = v.y / len,
        .z = v.z / len,
    };
}

Vec3 vec3_cross(Vec3 side1, Vec3 side2) {
    return (Vec3) {
        .x = side1.y * side2.z - side1.z * side2.y,
        .y = side1.z * side2.x - side1.x * side2.z,
        .z = side1.x * side2.y - side1.y * side2.x
    };
}


Quaternion quat_add(Quaternion a, Quaternion b) {
    Quaternion q = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return q;
}

Quaternion quat_sub(Quaternion a, Quaternion b) {
    Quaternion q = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return q;
}

Quaternion quat_mul(Quaternion a, Quaternion b) {
    Quaternion q;
    q.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
    q.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    q.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
    q.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
    return q;
}

Quaternion quat_normalize(Quaternion q) {
    float len = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (len == 0.0f) return (Quaternion){0,0,0,1};
    float inv = 1.0f / len;
    return (Quaternion){ q.x*inv, q.y*inv, q.z*inv, q.w*inv };
}

Quaternion quat_from_axis_angle(Vec3 axis, float angle) {
    float half = angle * 0.5f;
    float s = sinf(half);
    Quaternion q = { axis.x * s, axis.y * s, axis.z * s, cosf(half) };
    return quat_normalize(q);
}

Quaternion quat_rotate(Vec3 direction, float angle) {
    return quat_from_axis_angle(direction, angle);
}

Mat4 quat_to_mat4(Quaternion q) {
    q = quat_normalize(q);

    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    Mat4 m;
    m.m00 = 1.0f - 2.0f * (yy + zz);
    m.m01 = 2.0f * (xy - wz);
    m.m02 = 2.0f * (xz + wy);
    m.m03 = 0.0f;

    m.m10 = 2.0f * (xy + wz);
    m.m11 = 1.0f - 2.0f * (xx + zz);
    m.m12 = 2.0f * (yz - wx);
    m.m13 = 0.0f;

    m.m20 = 2.0f * (xz - wy);
    m.m21 = 2.0f * (yz + wx);
    m.m22 = 1.0f - 2.0f * (xx + yy);
    m.m23 = 0.0f;

    m.m30 = 0.0f;
    m.m31 = 0.0f;
    m.m32 = 0.0f;
    m.m33 = 1.0f;

    return m;
}

Quaternion quat_inverse(Quaternion q) {
    float norm_sq = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (norm_sq == 0.0f) {
        return (Quaternion) {.0f, .0f, .0f, 1.f}; // return identity if invalid
    }

    float inv = 1.0f / norm_sq;
    return (Quaternion){
        -q.x * inv,
        -q.y * inv,
        -q.z * inv,
         q.w * inv
    };
}

File_t read_file(const char* filepath) {
    char* err_msg = NULL;
    FILE* fh      = fopen(filepath, "rb");

    if (!fh) goto error;

    size_t file_size = 0;
    if (fseek(fh, 0, SEEK_END)) goto error;

    file_size = ftell(fh);
    if (fseek(fh, 0, SEEK_SET)) goto error;
     
    char* data = (char*) malloc(file_size + 1);
    if (!data) {
        err_msg = "Couldn't allocate bytes to read file in RAM.";
        goto error;
    }

    if (fread(data, 1, file_size, fh) != file_size)
        goto error;

    data[file_size] = '\0';
    fclose(fh);

    return (File_t) {
        .data      = data,
        .size      = file_size + 1,
        .file_path = filepath
    };

error:
    char* error = strerror(errno);
    if (err_msg) error = err_msg;

    printf("read_file() Failed: %s\n", err_msg);

    if (fh) { fclose(fh); }
    return (File_t){ .data = NULL, .size = 0, .file_path = filepath };
}

GLuint createShaderProgramGL_(File_t vs_file, File_t fs_file) {
    GLuint vs = compile_shader(vs_file.data, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_file.data, GL_FRAGMENT_SHADER);
    GLuint quadProg = glCreateProgram();
    glAttachShader(quadProg, vs);
    glAttachShader(quadProg, fs);
    glLinkProgram(quadProg);
    return quadProg;
}

GLProgram_t createShaderProgramGL(File_t vs_file, File_t fs_file) {
    GLuint programID = createShaderProgramGL_(vs_file, fs_file);
    GLProgram_t program = {0};

    if (!programID) return program;

    program.program       = programID;
    program.world_mat_loc = glGetUniformLocation(programID, UNIFORM_WORLD_MATRIX);
    program.view_mat_loc  = glGetUniformLocation(programID, UNIFORM_VIEW_MATRIX);
    program.proj_mat_loc  = glGetUniformLocation(programID, UNIFORM_PROJ_MATRIX);
    return program;
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

void canvas_to_GLtexture(Olivec_Canvas src, GLint dest) {
    glBindTexture(GL_TEXTURE_2D, dest);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src.width, src.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, src.pixels);
    glBindTexture(GL_TEXTURE_2D, dest);    
}

Mesh_t createTriangleMesh(Vec3 v1, Vec3 v2, Vec3 v3, Color color, GLProgram_t prog) {
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
    Vec3 side1  = vec3_sub(v1, v2);
    Vec3 side2  = vec3_sub(v1, v3);
    Vec3 normal = vec3_norm(vec3_cross(side1, side2));

#   define NORMAL_
#   define UV_
#   define COLOR_

    float vbo_buffer[] = {
        v1.x, v1.y, v1.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 0.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        v2.x, v2.y, v2.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 1.0f, 0.0f, COLOR_ color.r, color.g, color.b,
        v3.x, v3.y, v3.z, NORMAL_ normal.x, normal.y, normal.z, UV_ 0.0f, 1.0f, COLOR_ color.r, color.g, color.b,
    };

    GLuint vao, vbo;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    
    if (!vao || !vbo) return (Mesh_t) {0};

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_buffer), vbo_buffer, GL_STATIC_DRAW);

    glBindVertexArray(vao);

    // use same buffer for all attributes.
    glVertexAttribPointer(ATTRIB_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);
    glVertexAttribPointer(ATTRIB_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));
    glVertexAttribPointer(ATTRIB_UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (6 * sizeof(float)));
    glVertexAttribPointer(ATTRIB_COLOR_LOCATION, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));

    glEnableVertexAttribArray(ATTRIB_POSITION_LOCATION);
    glEnableVertexAttribArray(ATTRIB_NORMAL_LOCATION);
    glEnableVertexAttribArray(ATTRIB_UV_LOCATION);
    glEnableVertexAttribArray(ATTRIB_COLOR_LOCATION);

    mesh.ebo          = 0;
    mesh.vbo          = vbo;
    mesh.vao          = vao;

    mesh.index_count  = 0;
    mesh.vertex_count = 3;

    return mesh;

#   undef NORMAL_
#   undef UV_
}


void renderMesh(Mesh_t m, Camera_t camera) {
    if (!m.program.program) return;

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

    glBindVertexArray(m.vao);
    glUseProgram(m.program.program);

    glUniformMatrix4fv(m.program.view_mat_loc, 1, GL_TRUE, (float*)(&camera.view_matrix));
    glUniformMatrix4fv(m.program.proj_mat_loc, 1, GL_TRUE, (float*)(&camera.proj_matrix));
    glUniformMatrix4fv(m.program.world_mat_loc, 1, GL_TRUE, (float*)(&m.localToWorld));
    
    if (m.ebo) {
        glDrawElements(GL_TRIANGLES, m.index_count, GL_UNSIGNED_SHORT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m.vertex_count);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}
