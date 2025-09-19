#pragma once

#define EPS 0.00001f

#define degtorad(x) M_PI * (x) / 180.0f
// mat lib
typedef struct Mat4_st Mat4;

struct Mat4_st {
    float m00, m01, m02, m03,
          m10, m11, m12, m13,
          m20, m21, m22, m23,
          m30, m31, m32, m33;
};

#define MATDEF static

MATDEF Mat4 mat_inv(Mat4 m);
MATDEF Mat4 mat_mul(Mat4 m1, Mat4 m2);
MATDEF Mat4 mat4_identity();
MATDEF Mat4 mat_translate(float x, float y, float z);
MATDEF Mat4 mat_scale(float x, float y, float z);
MATDEF Mat4 mat_rotate_x(float angle);
MATDEF Mat4 mat_rotate_y(float angle);
MATDEF Mat4 mat_rotate_z(float angle);

typedef struct vec3_st {
    float x, y, z;
} vec3;

typedef struct Color_st {
    float r, g, b, a;
} Color;

MATDEF vec3 mat_transform(vec3 v, Mat4 m, float v_w);

#define vec3_init(...) (vec3) {__VA_ARGS__}

vec3  vec3_add(vec3 a, vec3 b);
vec3  vec3_sub(vec3 a, vec3 b);
float vec3_dot(vec3 a, vec3 b);
vec3  vec3_scale(vec3 a, float s);
vec3  vec3_norm(vec3 v);
vec3  vec3_cross(vec3 side1, vec3 side2);

typedef struct Quaternion_st {
    float x, y, z, w;
} Quaternion;

Quaternion quat_add(Quaternion a, Quaternion b);
Quaternion quat_sub(Quaternion a, Quaternion b);
Quaternion quat_mul(Quaternion a, Quaternion b);
Quaternion quat_normalize(Quaternion q);
Quaternion quat_from_axis_angle(vec3 axis, float angle);
Quaternion quat_rotate(vec3 direction, float angle);
Mat4       quat_to_mat4(Quaternion q);
Quaternion quat_inverse(Quaternion q);

typedef enum rot_mode_enum {
    ROTMODE_EULER,
    ROTMODE_QUATERNION
} rot_mode_t;

#ifdef ENGINE_MATH_IMPLEMENTATION

// mat module

#include <stdbool.h>
#include <math.h>

typedef struct {
    float m00, m01, m02, m03,
          m10, m11, m12, m13,
          m20, m21, m22, m23,
          m30, m31, m32, m33;
} Mat4_st;

MATDEF Mat4 mat_inv(Mat4 m) {
    float inv[16];
    float det;
    float tmp[16] = {
        m.m00, m.m01, m.m02, m.m03,
        m.m10, m.m11, m.m12, m.m13,
        m.m20, m.m21, m.m22, m.m23,
        m.m30, m.m31, m.m32, m.m33
    };

    inv[0] =   tmp[5]  * tmp[10] * tmp[15] - 
               tmp[5]  * tmp[11] * tmp[14] - 
               tmp[9]  * tmp[6]  * tmp[15] + 
               tmp[9]  * tmp[7]  * tmp[14] +
               tmp[13] * tmp[6]  * tmp[11] - 
               tmp[13] * tmp[7]  * tmp[10];

    inv[4] =  -tmp[4]  * tmp[10] * tmp[15] + 
               tmp[4]  * tmp[11] * tmp[14] + 
               tmp[8]  * tmp[6]  * tmp[15] - 
               tmp[8]  * tmp[7]  * tmp[14] - 
               tmp[12] * tmp[6]  * tmp[11] + 
               tmp[12] * tmp[7]  * tmp[10];

    inv[8] =   tmp[4]  * tmp[9]  * tmp[15] - 
               tmp[4]  * tmp[11] * tmp[13] - 
               tmp[8]  * tmp[5]  * tmp[15] + 
               tmp[8]  * tmp[7]  * tmp[13] + 
               tmp[12] * tmp[5]  * tmp[11] - 
               tmp[12] * tmp[7]  * tmp[9];

    inv[12] = -tmp[4]  * tmp[9]  * tmp[14] + 
               tmp[4]  * tmp[10] * tmp[13] +
               tmp[8]  * tmp[5]  * tmp[14] - 
               tmp[8]  * tmp[6]  * tmp[13] - 
               tmp[12] * tmp[5]  * tmp[10] + 
               tmp[12] * tmp[6]  * tmp[9];

    inv[1] =  -tmp[1]  * tmp[10] * tmp[15] + 
               tmp[1]  * tmp[11] * tmp[14] + 
               tmp[9]  * tmp[2]  * tmp[15] - 
               tmp[9]  * tmp[3]  * tmp[14] - 
               tmp[13] * tmp[2]  * tmp[11] + 
               tmp[13] * tmp[3]  * tmp[10];

    inv[5] =   tmp[0]  * tmp[10] * tmp[15] - 
               tmp[0]  * tmp[11] * tmp[14] - 
               tmp[8]  * tmp[2]  * tmp[15] + 
               tmp[8]  * tmp[3]  * tmp[14] + 
               tmp[12] * tmp[2]  * tmp[11] - 
               tmp[12] * tmp[3]  * tmp[10];

    inv[9] =  -tmp[0]  * tmp[9]  * tmp[15] + 
               tmp[0]  * tmp[11] * tmp[13] + 
               tmp[8]  * tmp[1]  * tmp[15] - 
               tmp[8]  * tmp[3]  * tmp[13] - 
               tmp[12] * tmp[1]  * tmp[11] + 
               tmp[12] * tmp[3]  * tmp[9];

    inv[13] =  tmp[0]  * tmp[9]  * tmp[14] - 
               tmp[0]  * tmp[10] * tmp[13] - 
               tmp[8]  * tmp[1]  * tmp[14] + 
               tmp[8]  * tmp[2]  * tmp[13] + 
               tmp[12] * tmp[1]  * tmp[10] - 
               tmp[12] * tmp[2]  * tmp[9];

    inv[2] =   tmp[1]  * tmp[6]  * tmp[15] - 
               tmp[1]  * tmp[7]  * tmp[14] - 
               tmp[5]  * tmp[2]  * tmp[15] + 
               tmp[5]  * tmp[3]  * tmp[14] + 
               tmp[13] * tmp[2]  * tmp[7]  - 
               tmp[13] * tmp[3]  * tmp[6];

    inv[6] =  -tmp[0]  * tmp[6]  * tmp[15] + 
               tmp[0]  * tmp[7]  * tmp[14] + 
               tmp[4]  * tmp[2]  * tmp[15] - 
               tmp[4]  * tmp[3]  * tmp[14] - 
               tmp[12] * tmp[2]  * tmp[7]  + 
               tmp[12] * tmp[3]  * tmp[6];

    inv[10] =  tmp[0]  * tmp[5]  * tmp[15] - 
               tmp[0]  * tmp[7]  * tmp[13] - 
               tmp[4]  * tmp[1]  * tmp[15] + 
               tmp[4]  * tmp[3]  * tmp[13] + 
               tmp[12] * tmp[1]  * tmp[7]  - 
               tmp[12] * tmp[3]  * tmp[5];

    inv[14] = -tmp[0]  * tmp[5]  * tmp[14] + 
               tmp[0]  * tmp[6]  * tmp[13] + 
               tmp[4]  * tmp[1]  * tmp[14] - 
               tmp[4]  * tmp[2]  * tmp[13] - 
               tmp[12] * tmp[1]  * tmp[6]  + 
               tmp[12] * tmp[2]  * tmp[5];

    inv[3] =  -tmp[1]  * tmp[6]  * tmp[11] + 
               tmp[1]  * tmp[7]  * tmp[10] + 
               tmp[5]  * tmp[2]  * tmp[11] - 
               tmp[5]  * tmp[3]  * tmp[10] - 
               tmp[9]  * tmp[2]  * tmp[7]  + 
               tmp[9]  * tmp[3]  * tmp[6];

    inv[7] =   tmp[0]  * tmp[6]  * tmp[11] - 
               tmp[0]  * tmp[7]  * tmp[10] - 
               tmp[4]  * tmp[2]  * tmp[11] + 
               tmp[4]  * tmp[3]  * tmp[10] + 
               tmp[8]  * tmp[2]  * tmp[7]  - 
               tmp[8]  * tmp[3]  * tmp[6];

    inv[11] = -tmp[0]  * tmp[5]  * tmp[11] + 
               tmp[0]  * tmp[7]  * tmp[9]  + 
               tmp[4]  * tmp[1]  * tmp[11] - 
               tmp[4]  * tmp[3]  * tmp[9]  - 
               tmp[8]  * tmp[1]  * tmp[7]  + 
               tmp[8]  * tmp[3]  * tmp[5];

    inv[15] =  tmp[0]  * tmp[5]  * tmp[10] - 
               tmp[0]  * tmp[6]  * tmp[9]  - 
               tmp[4]  * tmp[1]  * tmp[10] + 
               tmp[4]  * tmp[2]  * tmp[9]  + 
               tmp[8]  * tmp[1]  * tmp[6]  - 
               tmp[8]  * tmp[2]  * tmp[5];

    det = tmp[0] * inv[0] + tmp[1] * inv[4] + tmp[2] * inv[8] + tmp[3] * inv[12];

    if (fabsf(det) < 1e-6f) {
        return (Mat4) {0.0f};
    }

    det = 1.0f / det;

    m.m00 = inv[0]  * det;  m.m01 = inv[1]  * det;  m.m02 = inv[2]  * det;  m.m03 = inv[3]  * det;
    m.m10 = inv[4]  * det;  m.m11 = inv[5]  * det;  m.m12 = inv[6]  * det;  m.m13 = inv[7]  * det;
    m.m20 = inv[8]  * det;  m.m21 = inv[9]  * det;  m.m22 = inv[10] * det;  m.m23 = inv[11] * det;
    m.m30 = inv[12] * det;  m.m31 = inv[13] * det;  m.m32 = inv[14] * det;  m.m33 = inv[15] * det;

    return m;
}

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

MATDEF vec3 mat_transform(vec3 v, Mat4 m, float v_w) {
    // assumes w = 1
    return (vec3) {
        .x = m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v_w,
        .y = m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v_w,
        .z = m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v_w,
    };
}

vec3 vec3_add(vec3 a, vec3 b) {
    return (vec3) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;  
}

vec3 vec3_scale(vec3 a, float s) {
    return (vec3) {.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

vec3 vec3_norm(vec3 v) {
    float len = v.x*v.x + v.y*v.y + v.z*v.z;
    if (fabsf(len) < EPS) return (vec3) {0.0f};
    
    len = sqrtf(len);

    return (vec3) {
        .x = v.x / len,
        .y = v.y / len,
        .z = v.z / len,
    };
}

vec3 vec3_cross(vec3 side1, vec3 side2) {
    return (vec3) {
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

Quaternion quat_from_axis_angle(vec3 axis, float angle) {
    float half = angle * 0.5f;
    float s = sinf(half);
    Quaternion q = { axis.x * s, axis.y * s, axis.z * s, cosf(half) };
    return quat_normalize(q);
}

Quaternion quat_rotate(vec3 direction, float angle) {
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

#endif // ENGINE_MATH_IMPLEMENTATION