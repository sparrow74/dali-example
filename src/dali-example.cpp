/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define LOG_TAG "DALI_NATIVEGL_LIBRARY"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <tbm_surface_queue.h>

#include <dali-toolkit/dali-toolkit.h>
#include <dali/devel-api/adaptor-framework/gl-window.h>
#include <dali/devel-api/adaptor-framework/native-gl-extensions.h>
#include <dali/devel-api/adaptor-framework/window-devel.h>
#include <dali/integration-api/debug.h>

using namespace Dali;
using Dali::Toolkit::TextLabel;

static Dali::GlWindow mGLWindow;

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define ENABLE_GL_OFFSCREEN 0

const Vector4 WINDOW_COLOR(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef __GNUC__
#if __GNUC__ >= 4
#ifndef API
#define API __attribute__((visibility("default")))
#endif
#endif
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

typedef struct
{
  float x, y;
} FloatPoint;

typedef struct _matrix matrix_t;
struct _matrix
{
  GLfloat m[4][4];
};

/* Application data */
typedef struct GLDATA
{
  bool mouse_down;

  int windowAngle;

  bool initialized;

  int width;
  int height;

  FloatPoint anglePoint;
  FloatPoint curPoint;
  FloatPoint prevPoint;

  ////////////////////////////////////////////////////////////////////////
  // OnScreen

  ////////////////////////////////////////////////////////////////////////
  /// egl Image Extension
  ///
  int mCount;
  Dali::NativeGlExtensions::ImageExtension mImageExtension;
  Dali::NativeGlExtensions::SyncExtension mSyncExtension;

  ////////////////////////////////////////////////////////////////////////
  /// Offscreen
  ///
  tbm_bufmgr mTbmDisplay;
  tbm_surface_queue_h mTbmSurfaceQueue;
  tbm_surface_h mCurrentSurface;

  bool IsCreatedEGLImage;
  bool IsCreatedSyncObject;

} GLData;
static GLData mGLData;

#define INF(fmt, arg...) ALOGI(fmt, ##arg)
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "evas_sample_app_5th"

#define ONEP +1.0f
#define ONEN -1.0f
#define ZERO 0.0f

static int NATIVE_BUFFER_WIDTH = 70;
static int NATIVE_BUFFER_HEIGHT = 50;

static bool isGL_OES_EGL_exist = true;

unsigned int program = 0;
unsigned int vtx_shader = 0;
unsigned int fgmt_shader = 0;
static unsigned int foregroundColor = 0xff0044ff;
static unsigned int backgroundColor = 0xff8844ff;

GLint idxVPosition;
GLint idxVTexCoord;
GLuint idxVBO_BOX;
GLuint idxTBO;
GLint idxMVP;
GLint idxMVP2;
GLint idxModelVeiw;
GLint idxfogColor;

float view[4][4];
float matPerspective[4][4];
float matModelview[4][4];
const float PI = 3.1415926535897932384626433832795f;
short _x;
short _y;
short _x1;
short _x2;
short _y1;
short _y2;
int g_angle = 0;

typedef struct nativeBufferTexture
{
  tbm_surface_h buffer;
  GLuint renderedTexture;
  unsigned int nativeBufferWidth;
  unsigned int nativeBufferHeight;
  int stride;
  unsigned char *ptr;
} nativeBufferTexture_s;

static nativeBufferTexture_s nativeTexture;

static const float VERTICES_BOX[] =
    {
        ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP, ONEP,
        ONEN, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEN, ONEN, ONEP, ONEP, ONEN,
        ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN, ONEN, ONEN, ONEP, ONEN,
        ONEP, ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP,
        ONEN, ONEP, ONEP, ONEP, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN,
        ONEN, ONEN, ONEP, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN};

static const float TEXTURE_COORD[] =
    {
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
        ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP};

static const char VERTEX_TEXT[] =
    "attribute vec3 a_position;\n"
    "attribute vec2 a_texcoord;\n"
    "uniform mat4 u_mvpMatrix;\n"
    "uniform mat4 u_modelMatrix;\n"
    "varying vec2 v_texcoord;\n"
    "varying float fogFactor;\n"
    "void main()\n"
    "{\n"
    "	v_texcoord = a_texcoord;\n"
    "	vec3 pos = vec3( u_modelMatrix * vec4(a_position, 1.0) );\n"
    "	fogFactor = clamp( ( 6.5 - length(pos) ) / 2.5, 0.0, 1.0);\n"
    "	gl_Position = u_mvpMatrix * vec4(a_position, 1.0);\n"
    "}\n";

static const char FRAGMENT_TEXT[] =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec4 u_fogColor;\n"
    "varying vec2 v_texcoord;\n"
    "varying float fogFactor;\n"
    "void main (void)\n"
    "{\n"
    "	vec4 resColor;\n"
    "	resColor =  vec4(texture2D(uTexture, vec2(1,1) - v_texcoord).rgb, 1.0);\n"
    "	gl_FragColor = mix( u_fogColor, resColor, fogFactor );\n"
    "}\n";

static const char FRAGMENT_TEXT_GL_OES_EGL_IMAGE[] =
    "#extension GL_OES_EGL_image_external : require\n"
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform samplerExternalOES uTexture;\n"
    "uniform vec4 u_fogColor;\n"
    "varying vec2 v_texcoord;\n"
    "varying float fogFactor;\n"
    "void main (void)\n"
    "{\n"
    "	vec4 resColor;\n"
    "	resColor =  vec4(texture2D(uTexture, vec2(1,1) - v_texcoord).rgb, 1.0);\n"
    "	gl_FragColor = mix( u_fogColor, resColor, fogFactor );\n"
    "}\n";

const unsigned char FONT8x8[97][8] = {
    {0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, // columns, rows, num_bytes_per_char
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space 0x20
    {0x30, 0x78, 0x78, 0x30, 0x30, 0x00, 0x30, 0x00}, // !
    {0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00}, // #
    {0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00}, // $
    {0x00, 0x63, 0x66, 0x0C, 0x18, 0x33, 0x63, 0x00}, // %
    {0x1C, 0x36, 0x1C, 0x3B, 0x6E, 0x66, 0x3B, 0x00}, // &
    {0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00}, // (
    {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00}, // )
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // *
    {0x00, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x00, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30}, // ,
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // / (forward slash)
    {0x3E, 0x63, 0x63, 0x6B, 0x63, 0x63, 0x3E, 0x00}, // 0 0x30
    {0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1
    {0x3C, 0x66, 0x06, 0x1C, 0x30, 0x66, 0x7E, 0x00}, // 2
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, // 3
    {0x0E, 0x1E, 0x36, 0x66, 0x7F, 0x06, 0x0F, 0x00}, // 4
    {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, // 5
    {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, // 6
    {0x7E, 0x66, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x00}, // 7
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, // 8
    {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00}, // 9
    {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00}, // :
    {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30}, // ;
    {0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00}, // <
    {0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00}, // =
    {0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00}, // >
    {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x00, 0x18, 0x00}, // ?
    {0x3E, 0x63, 0x6F, 0x69, 0x6F, 0x60, 0x3E, 0x00}, // @ 0x40
    {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, // A
    {0x7E, 0x33, 0x33, 0x3E, 0x33, 0x33, 0x7E, 0x00}, // B
    {0x1E, 0x33, 0x60, 0x60, 0x60, 0x33, 0x1E, 0x00}, // C
    {0x7C, 0x36, 0x33, 0x33, 0x33, 0x36, 0x7C, 0x00}, // D
    {0x7F, 0x31, 0x34, 0x3C, 0x34, 0x31, 0x7F, 0x00}, // E
    {0x7F, 0x31, 0x34, 0x3C, 0x34, 0x30, 0x78, 0x00}, // F
    {0x1E, 0x33, 0x60, 0x60, 0x67, 0x33, 0x1F, 0x00}, // G
    {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // H
    {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // I
    {0x0F, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00}, // J
    {0x73, 0x33, 0x36, 0x3C, 0x36, 0x33, 0x73, 0x00}, // K
    {0x78, 0x30, 0x30, 0x30, 0x31, 0x33, 0x7F, 0x00}, // L
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // M
    {0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, 0x63, 0x00}, // N
    {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00}, // O
    {0x7E, 0x33, 0x33, 0x3E, 0x30, 0x30, 0x78, 0x00}, // P 0x50
    {0x3C, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x0E, 0x00}, // Q
    {0x7E, 0x33, 0x33, 0x3E, 0x36, 0x33, 0x73, 0x00}, // R
    {0x3C, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x3C, 0x00}, // S
    {0x7E, 0x5A, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // T
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7E, 0x00}, // U
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // X
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x00}, // Y
    {0x7F, 0x63, 0x46, 0x0C, 0x19, 0x33, 0x7F, 0x00}, // Z
    {0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00}, // [
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // \ (back slash)
    {0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00}, // ]
    {0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // _
    {0x18, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00}, // ` 0x60
    {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3B, 0x00}, // a
    {0x70, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00}, // b
    {0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}, // c
    {0x0E, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3B, 0x00}, // d
    {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00}, // e
    {0x1C, 0x36, 0x30, 0x78, 0x30, 0x30, 0x78, 0x00}, // f
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x7C}, // g
    {0x70, 0x30, 0x36, 0x3B, 0x33, 0x33, 0x73, 0x00}, // h
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00}, // i
    {0x06, 0x00, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C}, // j
    {0x70, 0x30, 0x33, 0x36, 0x3C, 0x36, 0x73, 0x00}, // k
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // l
    {0x00, 0x00, 0x66, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // m
    {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, // n
    {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00}, // o
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // p
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // q
    {0x00, 0x00, 0x6E, 0x3B, 0x33, 0x30, 0x78, 0x00}, // r
    {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}, // s
    {0x08, 0x18, 0x3E, 0x18, 0x18, 0x1A, 0x0C, 0x00}, // t
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3B, 0x00}, // u
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // v
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // w
    {0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // x
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x7C}, // y
    {0x00, 0x00, 0x7E, 0x4C, 0x18, 0x32, 0x7E, 0x00}, // z
    {0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00}, // {
    {0x0C, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0x00}, // |
    {0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00}, // }
    {0x3B, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ~
    {0x1C, 0x36, 0x36, 0x1C, 0x00, 0x00, 0x00, 0x00}  // DEL
};

char *tizen_str = (char *)"Tizen";
char *dali_str = (char *)"DALi";

Dali::NativeImageSourcePtr mNativeImage;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// common

static void
init_matrix(float matrix[16])
{
  matrix[0] = 1.0f;
  matrix[1] = 0.0f;
  matrix[2] = 0.0f;
  matrix[3] = 0.0f;

  matrix[4] = 0.0f;
  matrix[5] = 1.0f;
  matrix[6] = 0.0f;
  matrix[7] = 0.0f;

  matrix[8] = 0.0f;
  matrix[9] = 0.0f;
  matrix[10] = 1.0f;
  matrix[11] = 0.0f;

  matrix[12] = 0.0f;
  matrix[13] = 0.0f;
  matrix[14] = 0.0f;
  matrix[15] = 1.0f;
}

static void
multiply_matrix(float matrix[16], const float matrix0[16], const float matrix1[16])
{
  int i;
  int row;
  int column;
  float temp[16];

  for (column = 0; column < 4; column++)
  {
    for (row = 0; row < 4; row++)
    {
      temp[(column * 4) + row] = 0.0f;

      for (i = 0; i < 4; i++)
        temp[(column * 4) + row] += matrix0[(i * 4) + row] * matrix1[(column * 4) + i];
    }
  }

  for (i = 0; i < 16; i++)
    matrix[i] = temp[i];
}

void Frustum(float matrix[4][4], float left, float right, float bottom, float top, float near, float far)
{
  float diffX = right - left;
  float diffY = top - bottom;
  float diffZ = far - near;

  if ((near <= 0.0f) || (far <= 0.0f) ||
      (diffX <= 0.0f) || (diffY <= 0.0f) || (diffZ <= 0.0f))
  {
    return;
  }

  matrix[0][0] = 2.0f * near / diffX;
  matrix[1][1] = 2.0f * near / diffY;
  matrix[2][0] = (right + left) / diffX;
  matrix[2][1] = (top + bottom) / diffY;
  matrix[2][2] = -(near + far) / diffZ;
  matrix[2][3] = -1.0f;
  matrix[3][2] = -2.0f * near * far / diffZ;

  matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0f;
  matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0f;
  matrix[3][0] = matrix[3][1] = matrix[3][3] = 0.0f;
}

void Perspective(float pResult[4][4], float fovY, float aspect, float near, float far)
{
  float fovRadian = fovY / 360.0f * PI;
  float top = tanf(fovRadian) * near;
  float right = top * aspect;

  Frustum(pResult, -right, right, -top, top, near, far);
}

void Translate(float pResult[4][4], float tx, float ty, float tz)
{
  pResult[3][0] += (pResult[0][0] * tx + pResult[1][0] * ty + pResult[2][0] * tz);
  pResult[3][1] += (pResult[0][1] * tx + pResult[1][1] * ty + pResult[2][1] * tz);
  pResult[3][2] += (pResult[0][2] * tx + pResult[1][2] * ty + pResult[2][2] * tz);
  pResult[3][3] += (pResult[0][3] * tx + pResult[1][3] * ty + pResult[2][3] * tz);
}

void Rotate(float pResult[4][4], float angle, float x, float y, float z)
{
  float rotate[4][4];

  float cos = cosf(angle * PI / 180.0f);
  float sin = sinf(angle * PI / 180.0f);
  float cos1 = 1.0f - cos;

  float len = sqrtf(x * x + y * y + z * z);

  x = x / len;
  y = y / len;
  z = z / len;

  rotate[0][0] = (x * x) * cos1 + cos;
  rotate[0][1] = (x * y) * cos1 - z * sin;
  rotate[0][2] = (z * x) * cos1 + y * sin;
  rotate[0][3] = 0.0f;

  rotate[1][0] = (x * y) * cos1 + z * sin;
  rotate[1][1] = (y * y) * cos1 + cos;
  rotate[1][2] = (y * z) * cos1 - x * sin;
  rotate[1][3] = 0.0f;

  rotate[2][0] = (z * x) * cos1 - y * sin;
  rotate[2][1] = (y * z) * cos1 + x * sin;
  rotate[2][2] = (z * z) * cos1 + cos;

  rotate[2][3] = rotate[3][0] = rotate[3][1] = rotate[3][2] = 0.0f;
  rotate[3][3] = 1.0f;

  multiply_matrix((float *)pResult, (float *)pResult, (float *)rotate);
}

static void
characterWindow(int x, int y, int w, int h)
{
  // current pixel location
  _x = x;
  _y = y;
  // characterWindow settings
  _x1 = x;
  _x2 = x + w - 1;
  _y1 = y;
  _y2 = y + h - 1;
}

static void
putp(nativeBufferTexture_s *ns, int colour)
{
  unsigned int *_p;
  _p = (unsigned int *)(((unsigned char *)ns->ptr) + (_y * ns->stride));
  _p[_x] = colour;

  _x++;
  if (_x > _x2)
  {
    _x = _x1;
    _y++;
    if (_y > _y2)
    {
      _y = _y1;
    }
  }
}

static void
blitbit(nativeBufferTexture_s *ns, int x, int y, int w, int h, const char *colour)
{
  int i;
  characterWindow(x, y, w, h);
  for (i = 0; i < w * h; i++)
  {
    char byte = colour[i >> 3];
    int offset = i & 0x7;
    int c = ((byte << offset) & 0x80) ? foregroundColor : backgroundColor;
    putp(ns, c);
  }
}

static void
character(nativeBufferTexture_s *ns, int column, int row, int value)
{
  blitbit(ns, column * 8, row * 8, 8, 8, (char *)&(FONT8x8[value - 0x1F][0]));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// offscreen

static void
updateNativeTexture(nativeBufferTexture_s *nativeTexture, unsigned int column, unsigned int row, char *str)
{
  unsigned int x, y;
  unsigned int *_p;
  tbm_surface_info_s surface_info;

  tbm_surface_map(nativeTexture->buffer, TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE, &surface_info);

  if (surface_info.num_planes != 0)
  {
    nativeTexture->ptr = surface_info.planes[0].ptr;
    nativeTexture->stride = surface_info.planes[0].stride;

    fprintf(stderr, "native buffer width: %d, height: %d, stride: %d\n", nativeTexture->nativeBufferWidth, nativeTexture->nativeBufferHeight, nativeTexture->stride);

    for (y = 0; y < nativeTexture->nativeBufferHeight; y++)
    {
      _p = (unsigned int *)(((unsigned char *)nativeTexture->ptr) + (y * nativeTexture->stride));
      for (x = 0; x < nativeTexture->nativeBufferWidth; x++)
      {
        _p[x] = backgroundColor;
      }
    }

    while (*str != '\0')
    {
      character(nativeTexture, column++, row, (int)*str++);
    }
  }

  tbm_surface_unmap(nativeTexture->buffer);
}

void init()
{
  //    tbm_format *formats;
  //    tbm_format format = TBM_FORMAT_ARGB8888;
  //    uint32_t formats_count;

  g_angle = 0.0f;
  const char *p;
  p = VERTEX_TEXT;
  vtx_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vtx_shader, 1, &p, NULL);
  glCompileShader(vtx_shader);
  const GLubyte *ext = glGetString(GL_EXTENSIONS);
  if (strstr((const char *)ext, "GL_OES_EGL_image_external") == NULL)
  {
    printf("GL_OES_EGL_image_external is required in order to fully get effect from API. %i", __LINE__);
    isGL_OES_EGL_exist = false;
  }
  p = (isGL_OES_EGL_exist) ? FRAGMENT_TEXT_GL_OES_EGL_IMAGE : FRAGMENT_TEXT;
  fgmt_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fgmt_shader, 1, &p, NULL);
  glCompileShader(fgmt_shader);

  program = glCreateProgram();
  glAttachShader(program, vtx_shader);
  glAttachShader(program, fgmt_shader);
  glLinkProgram(program);

  idxVPosition = glGetAttribLocation(program, "a_position");
  idxVTexCoord = glGetAttribLocation(program, "a_texcoord");
  idxMVP = glGetUniformLocation(program, "u_mvpMatrix");
  idxModelVeiw = glGetUniformLocation(program, "u_modelMatrix");
  idxfogColor = glGetUniformLocation(program, "u_fogColor");

  glGenBuffers(1, &idxVBO_BOX);
  glBindBuffer(GL_ARRAY_BUFFER, idxVBO_BOX);
  glBufferData(GL_ARRAY_BUFFER, 12 * 6 * sizeof(float), VERTICES_BOX, GL_STATIC_DRAW);

  glGenBuffers(1, &idxTBO);
  glBindBuffer(GL_ARRAY_BUFFER, idxTBO);
  glBufferData(GL_ARRAY_BUFFER, 6 * 8 * sizeof(float), TEXTURE_COORD, GL_STATIC_DRAW);

  nativeTexture.nativeBufferWidth = NATIVE_BUFFER_WIDTH;
  nativeTexture.nativeBufferHeight = NATIVE_BUFFER_HEIGHT;

#if 0
    tbm_surface_query_formats(&formats, &formats_count);
    for (int i = 0; i < (int)formats_count; i++)
    {
        if (formats[i] == TBM_FORMAT_BGRA8888)
            format = TBM_FORMAT_BGRA8888;
    }
#endif
  //nativeTexture.buffer = tbm_surface_create(nativeTexture.nativeBufferWidth, nativeTexture.nativeBufferHeight, format);
  nativeTexture.buffer = AnyCast<tbm_surface_h>(mNativeImage->GetNativeImageSource());

  updateNativeTexture(&nativeTexture, 1, 3, tizen_str);

  glGenTextures(1, &nativeTexture.renderedTexture);

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeTexture.renderedTexture);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  mGLData.mImageExtension.CreateImageKHR(SCREEN_WIDTH, SCREEN_HEIGHT, NativeImageSource::ColorDepth::COLOR_DEPTH_32, nativeTexture.buffer);
  mGLData.mImageExtension.TargetTextureKHR();

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);

  init_matrix((float *)view);
  init_matrix((float *)matPerspective);
  Perspective(matPerspective, 60.0f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 1.0f, 400.0f);
}

// pullic Callbacks
// intialize callback that gets called once for intialization
API void initialize_gl()
{
  fprintf(stderr, "%s: initialize_gl start~~~~\n", __FUNCTION__);

  mGLData.mImageExtension = Dali::NativeGlExtensions::ImageExtension::New(mGLWindow);
  mGLData.mSyncExtension = Dali::NativeGlExtensions::SyncExtension::New(mGLWindow);

  init();

  mGLData.mCount = 0;
}

// draw callback is where all the main GL rendering happens
API int renderFrame_gl()
{
  int w = SCREEN_WIDTH;
  int h = SCREEN_HEIGHT;
  static double hue = 0.0;

  g_angle += 1.0f;

  if (g_angle >= 360.0f)
  {
    g_angle -= 360.0f;
  }

  g_angle = (g_angle + 1) % (360 * 3);

  float r = (1.0f + sin(hue - 2.0 * PI / 3.0)) / 3.0f;
  float g = (1.0f + sin(hue)) / 3.0f;
  float b = (1.0f + sin(hue + 2.0 * PI / 3.0)) / 3.0f;

  hue += 0.03;

  glUseProgram(program);
  glUniform4f(idxfogColor, r, g, b, 1.0f);
  glViewport(0, 0, w, h);
  glClearColor(r, g, b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  init_matrix((float *)matModelview);
  Translate(matModelview, 0.0f, 0.0, -6.0);

  Rotate(matModelview, -(float)g_angle / 3.0f, 0.0, 0.0, 1.0);
  Rotate(matModelview, -g_angle, 0.0, 1.0, 0.0);

  multiply_matrix((float *)view, (float *)matPerspective, (float *)matModelview);
  glUniformMatrix4fv(idxModelVeiw, 1, GL_FALSE, (GLfloat *)(float *)matModelview);
  glUniformMatrix4fv(idxMVP, 1, GL_FALSE, (GLfloat *)view);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeTexture.renderedTexture);
  glActiveTexture(GL_TEXTURE0);
  glBindBuffer(GL_ARRAY_BUFFER, idxVBO_BOX);
  glVertexAttribPointer(idxVPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
  glEnableVertexAttribArray(idxVPosition);
  glBindBuffer(GL_ARRAY_BUFFER, idxTBO);
  glVertexAttribPointer(idxVTexCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
  glEnableVertexAttribArray(idxVTexCoord);

  for (int i = 0; i < 6; i++)
  {
    glDrawArrays(GL_TRIANGLE_STRIP, 4 * i, 4);
  }

  if (mGLData.IsCreatedSyncObject == false)
  {
    mGLData.mSyncExtension.CreateSyncObject();
    mGLData.IsCreatedSyncObject = true;
    DALI_LOG_ERROR("Create Sync Object!!\n");
  }

  glDisableVertexAttribArray(idxVPosition);
  glDisableVertexAttribArray(idxVTexCoord);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  return 1;
}

// delete callback gets called when glview is deleted
API void terminate_gl()
{
  tbm_surface_destroy(nativeTexture.buffer);
  mGLData.mImageExtension.DestroyImageKHR();

  glDeleteShader(vtx_shader);
  glDeleteShader(fgmt_shader);
  glDeleteProgram(program);
  glDeleteBuffers(1, &idxTBO);
  glDeleteBuffers(1, &idxVBO_BOX);
  glDeleteTextures(1, &nativeTexture.renderedTexture);
}

// This example shows how to create and display Hello World! using a simple TextActor
//
class HelloWorldController : public ConnectionTracker
{
public:
  HelloWorldController(Application &application)
      : mApplication(application)
  {
    // Connect to the Application's Init signal
    mApplication.InitSignal().Connect(this, &HelloWorldController::Create);
  }

  ~HelloWorldController()
  {
    // Nothing to do here;
  }

  // The Init signal is received once (only) during the Application lifetime
  void Create(Application &application)
  {
    mGLData.initialized = false;

    mNativeImage = Dali::NativeImageSource::New(NATIVE_BUFFER_WIDTH, NATIVE_BUFFER_HEIGHT, Dali::NativeImageSource::ColorDepth::COLOR_DEPTH_32);

    mGLWindow = Dali::GlWindow::New(PositionSize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), "GLWindowSample", "", false);
    mGLWindow.SetEglConfig(true, true, 0, Dali::GlWindow::GlesVersion::VERSION_2_0);
    mGLWindow.RegisterGlCallback(Dali::MakeCallback(initialize_gl),
                                 Dali::MakeCallback(renderFrame_gl),
                                 Dali::MakeCallback(terminate_gl));

    mGLData.width = SCREEN_WIDTH;
    mGLData.height = SCREEN_HEIGHT;

    mGLData.IsCreatedEGLImage = false;
    mGLData.IsCreatedSyncObject = false;

    mGLWindow.Show();

    mGLWindow.ResizeSignal().Connect(this, &HelloWorldController::OnGLWindowResize);

    mGLWindow.TouchedSignal().Connect(this, &HelloWorldController::OnGLWindowTouch);
    mGLWindow.KeyEventSignal().Connect(this, &HelloWorldController::OnGLWindowKeyEvent);

    DALI_LOG_ERROR("Create Timer~~~\n");
    mTimer = Timer::New(300);
    mTimer.TickSignal().Connect(this, &HelloWorldController::OnTimerTick);
    mTimer.Start();
  }

  bool OnTimerTick()
  {
    DALI_LOG_ERROR("OnTimerTick()!!! time out\n");
    if (mGLData.IsCreatedSyncObject)
    {
      bool ret = mGLData.mSyncExtension.IsSynced();
      DALI_LOG_ERROR("the return of IsSynced: %d\n", ret);
      mGLData.mSyncExtension.DestroySyncObject();
      mGLData.IsCreatedSyncObject = false;
    }

    mGLData.mCount++;
    if (mGLData.mCount % 100 == 0)
    {
      updateNativeTexture(&nativeTexture, 1, 3, dali_str);
    }
    if (mGLData.mCount % 200 == 0)
    {
      updateNativeTexture(&nativeTexture, 1, 3, tizen_str);
    }
    return true;
  }

  void OnWindowResize(Dali::Window winHandle, Dali::Window::WindowSize size)
  {
    int width = size.GetWidth();
    int height = size.GetHeight();
    mTextLabel.SetProperty(Actor::Property::SIZE, Vector2(width, 200));
    DALI_LOG_ERROR("OnWindowResized, current orientation:%d, width:%d, height:%d\n", DevelWindow::GetCurrentOrientation(winHandle), width, height);
  }

  bool OnTouch(Actor actor, const TouchEvent &touch)
  {
    return true;
  }

  void OnKeyEvent(const KeyEvent &event)
  {
    if (event.GetState() == KeyEvent::DOWN)
    {
      if (IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
      {
        mApplication.Quit();
      }
    }
  }

  void OnGLWindowResize(Dali::GlWindow::WindowSize size)
  {
  }

  void OnGLWindowTouch(const TouchEvent &touch)
  {
    DALI_LOG_ERROR("touch.GetState(), %d\n", touch.GetState(0));
    if (touch.GetState(0) == 0)
    {
      mGLData.mouse_down = true;
      DALI_LOG_ERROR("DOWN, x:%f, y:%f\n", touch.GetScreenPosition(0).x, touch.GetScreenPosition(0).y);
    }
    else if (touch.GetState(0) == 1)
    {
      mGLData.mouse_down = false;
      DALI_LOG_ERROR("UP, x:%f, y:%f\n", touch.GetScreenPosition(0).x, touch.GetScreenPosition(0).y);
    }
    else if (touch.GetState(0) == 2)
    {
      float dx = 0;
      float dy = 0;
      mGLData.curPoint.x = touch.GetScreenPosition(0).x;
      mGLData.curPoint.y = touch.GetScreenPosition(0).y;
      DALI_LOG_ERROR("MOTION x:%f, y:%f\n", mGLData.curPoint.x, mGLData.curPoint.y);
      if (mGLData.mouse_down == true)
      {
        dx = mGLData.curPoint.x - mGLData.prevPoint.x;
        dy = mGLData.curPoint.y - mGLData.prevPoint.y;
        mGLData.anglePoint.x += dy;
        mGLData.anglePoint.y += dx;
      }
      mGLData.prevPoint.x = mGLData.curPoint.x;
      mGLData.prevPoint.y = mGLData.curPoint.y;
    }
    return;
  }

  void OnGLWindowKeyEvent(const KeyEvent &event)
  {
    if (event.GetState() == KeyEvent::DOWN)
    {
      if (IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
      {
        mApplication.Quit();
        mUIWindow.Raise();
      }
    }
  }

private:
  Application &mApplication;
  Dali::Window mUIWindow;
  TextLabel mTextLabel;

  Timer mTimer;
};

int DALI_EXPORT_API main(int argc, char **argv)
{
  Application application = Application::New(&argc, &argv);
  HelloWorldController test(application);
  application.MainLoop();
  return 0;
}
