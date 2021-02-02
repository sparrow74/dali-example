/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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
#include <dali/devel-api/adaptor-framework/gl-window-extensions.h>
#include <dali/devel-api/adaptor-framework/window-devel.h>
#include <dali/integration-api/debug.h>


using namespace Dali;
using Dali::Toolkit::TextLabel;

static Dali::GlWindow  mGLWindow;

#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           720

#define ENABLE_GL_OFFSCREEN     0

const Vector4 WINDOW_COLOR(0.0f, 0.0f, 0.0f, 0.0f );

#ifdef __GNUC__
# if __GNUC__ >= 4
#  ifndef API
#   define API __attribute__ ((visibility("default")))
#  endif
# endif
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

typedef struct {
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

    GLuint onscreen_programObject;

    // Attribute locations
    GLint  positionLoc;
    GLint  texCoordLoc;

    // Sampler location
    GLint samplerLoc;

    GLint mvpLoc;
    // Texture handle
    GLuint textureId;

    matrix_t perspective;
    matrix_t mvp_matrix;

    ////////////////////////////////////////////////////////////////////////
    /// egl Image Extension
    ///
    int mCount;
    Dali::GlWindowExtensions::ImageExtension mImageExtension;
    Dali::GlWindowExtensions::SyncExtension mSyncExtension;

    ////////////////////////////////////////////////////////////////////////
    /// Offscreen
    ///
    tbm_bufmgr mTbmDisplay;
    tbm_surface_queue_h mTbmSurfaceQueue;
    tbm_surface_h mCurrentSurface;

    /// Window handle
    EGLNativeWindowType  hWnd;

    /// EGL display
    EGLDisplay  eglDisplay;

    /// EGL context
    EGLContext  eglContext;

    /// EGL surface
    EGLSurface  eglSurface;

    /// Offscreen program object
    GLuint offscreen_programObject;

    bool IsCreatedEGLImage;
    bool IsCreatedSyncObject;

} GLData;
static GLData mGLData;



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// common

static GLfloat vtxs[] =
{
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f
};

static GLfloat texcoords[] =
{
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};

static void
_matrix_load_identity(matrix_t *res)
{
    memset(res, 0x0, sizeof(matrix_t));

    res->m[0][0] = 1.0f;
    res->m[1][1] = 1.0f;
    res->m[2][2] = 1.0f;
    res->m[3][3] = 1.0f;
}

static void
_matrix_translate(matrix_t *res, GLfloat tx, GLfloat ty, GLfloat tz)
{
    res->m[3][0] += (res->m[0][0] * tx + res->m[1][0] * ty + res->m[2][0] * tz);
    res->m[3][1] += (res->m[0][1] * tx + res->m[1][1] * ty + res->m[2][1] * tz);
    res->m[3][2] += (res->m[0][2] * tx + res->m[1][2] * ty + res->m[2][2] * tz);
    res->m[3][3] += (res->m[0][3] * tx + res->m[1][3] * ty + res->m[2][3] * tz);
}

static void
_matrix_multiply(matrix_t *res, matrix_t *src_a, matrix_t *src_b)
{
    matrix_t tmp;
    int i;

    for( i = 0; i < 4; i++ )
    {
        tmp.m[i][0] = (src_a->m[i][0] * src_b->m[0][0]) +
            (src_a->m[i][1] * src_b->m[1][0]) +
            (src_a->m[i][2] * src_b->m[2][0]) +
            (src_a->m[i][3] * src_b->m[3][0]) ;

        tmp.m[i][1] = (src_a->m[i][0] * src_b->m[0][1]) +
            (src_a->m[i][1] * src_b->m[1][1]) +
            (src_a->m[i][2] * src_b->m[2][1]) +
            (src_a->m[i][3] * src_b->m[3][1]) ;

        tmp.m[i][2] = (src_a->m[i][0] * src_b->m[0][2]) +
            (src_a->m[i][1] * src_b->m[1][2]) +
            (src_a->m[i][2] * src_b->m[2][2]) +
            (src_a->m[i][3] * src_b->m[3][2]) ;

        tmp.m[i][3] = (src_a->m[i][0] * src_b->m[0][3]) +
            (src_a->m[i][1] * src_b->m[1][3]) +
            (src_a->m[i][2] * src_b->m[2][3]) +
            (src_a->m[i][3] * src_b->m[3][3]) ;
    }
    memcpy( res, &tmp, sizeof(matrix_t) );
}

static void
_matrix_scale(matrix_t *res, GLfloat sx, GLfloat sy, GLfloat sz)
{
    res->m[0][0] *= sx;
    res->m[0][1] *= sx;
    res->m[0][2] *= sx;
    res->m[0][3] *= sx;

    res->m[1][0] *= sy;
    res->m[1][1] *= sy;
    res->m[1][2] *= sy;
    res->m[1][3] *= sy;

    res->m[2][0] *= sz;
    res->m[2][1] *= sz;
    res->m[2][2] *= sz;
    res->m[2][3] *= sz;
}

static void
_matrix_frustum(matrix_t *res, float left, float right, float bottom,
                float top, float near, float far)
{
    float d_x = right - left;
    float d_y = top - bottom;
    float d_z = far - near;
    matrix_t frust;

    if ( (near <= 0.0f) || (far <= 0.0f) || (d_x <= 0.0f) || (d_y <= 0.0f) || (d_z <= 0.0f) ) return;

    frust.m[0][0] = 2.0f * near / d_x;
    frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

    frust.m[1][1] = 2.0f * near / d_y;
    frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

    frust.m[2][0] = (right + left) / d_x;
    frust.m[2][1] = (top + bottom) / d_y;
    frust.m[2][2] = -(near + far) / d_z;
    frust.m[2][3] = -1.0f;

    frust.m[3][2] = -2.0f * near * far / d_z;
    frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

    _matrix_multiply( res, &frust, res );
}

static void
_window_coord(matrix_t *mtx, int w, int h)
{
        float fovy, aspect, znear, zfar;
    float xmin, xmax, ymin, ymax;

    float z_camera;
//    float fovy_rad;

    glViewport( 0, 0, w, h );

    fovy = 60.0f;
    aspect = 1.0f;
    znear = 0.1f;
    zfar = 100.0f;

    ymax = znear * tan( fovy * M_PI / 360.0f );
    ymin = -ymax;
    xmax = ymax * aspect;
    xmin = ymin * aspect;

    _matrix_load_identity( mtx );
    _matrix_frustum( mtx, xmin, xmax, ymin, ymax, znear, zfar );

    z_camera = 0.866f;

    _matrix_translate( mtx, -0.5f, -0.5f, -z_camera );
    _matrix_scale( mtx, 1.0f/w, -1.0f/h, 1.0f/w );
    _matrix_translate( mtx, 0.0f, -1.0f*h, 0.0f );


//finish:

    return;
}

GLuint esLoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
    return 0;

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );

   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = (char*)malloc (sizeof(char) * infoLen );

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         printf ( "Error compiling shader:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}

GLuint esLoadProgram ( const char *vertShaderSrc, const char *fragShaderSrc )
{
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = esLoadShader ( GL_VERTEX_SHADER, vertShaderSrc );
   if ( vertexShader == 0 )
      return 0;

   fragmentShader = esLoadShader ( GL_FRAGMENT_SHADER, fragShaderSrc );
   if ( fragmentShader == 0 )
   {
      glDeleteShader( vertexShader );
      return 0;
   }

   // Create the program object
   programObject = glCreateProgram ( );

   if ( programObject == 0 )
      return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = (char*)malloc (sizeof(char) * infoLen );

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         printf ( "Error linking program:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return 0;
   }

   // Free up no longer needed shader resources
   glDeleteShader ( vertexShader );
   glDeleteShader ( fragmentShader );

   return programObject;
}

GLuint LoadShader ( GLenum type, char *shaderSrc )
{
    GLuint shader;
    GLint compiled;
    GLenum glErrorResult;

    // Create the shader object
    shader = glCreateShader ( type );

    if ( shader == 0 ) {
        if ((glErrorResult = glGetError()) != GL_NO_ERROR)
            printf("[glCreateShader][glGetError] 0x%04X\n", glErrorResult);
        return 0;
    }

    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glCreateShader][glGetError] 0x%04X\n", glErrorResult);
    printf("[glCreateShader] Success.\n");

    // Load the shader source
    glShaderSource ( shader, 1, &shaderSrc, NULL );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glShaderSource][glGetError] 0x%04X\n", glErrorResult);

    // Compile the shader
    glCompileShader ( shader );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glCompileShader][glGetError] 0x%04X\n", glErrorResult);

    // Check the compile status
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glGetShaderiv][glGetError] 0x%04X\n", glErrorResult);

    if ( !compiled )
    {
        GLint infoLen = 0;

        glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );

            glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            printf ( "[glGetShaderiv] Error compiling shader:\n%s\n", infoLog );

            free ( infoLog );
        }

        glDeleteShader ( shader );
        return 0;
    }

    return shader;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// onscreen

int Init_tex()
{
    const char vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   const char fShaderStr[] =
      "#extension GL_OES_EGL_image_external:require        \n"
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform samplerExternalOES s_texture;               \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "}                                                   \n";

    //GLint linked;

    //GLenum glErrorResult;

    _matrix_load_identity(&mGLData.perspective);
    _window_coord(&mGLData.perspective, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Load the shaders and get a linked program object
    mGLData.onscreen_programObject = esLoadProgram(vShaderStr, fShaderStr);

    // Get the attribute locations
    mGLData.positionLoc = glGetAttribLocation(mGLData.onscreen_programObject, "a_position");
    mGLData.texCoordLoc = glGetAttribLocation(mGLData.onscreen_programObject, "a_texCoord");
    mGLData.mvpLoc = glGetAttribLocation(mGLData.onscreen_programObject, "u_mvp_matrix");
    mGLData.samplerLoc = glGetUniformLocation (mGLData.onscreen_programObject, "s_texture");


    glDisable(GL_BLEND);

    glGenTextures(1, &mGLData.textureId);
    glBindTexture(GL_TEXTURE_2D, mGLData.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, (GLuint)NULL);

    glClearColor( 0.4f, 0.4f, 0.8f, 1.0f );

    glUseProgram(mGLData.onscreen_programObject);

    glVertexAttribPointer(mGLData.positionLoc, 3, GL_FLOAT, GL_FALSE, 0, vtxs);
    glEnableVertexAttribArray(mGLData.positionLoc);

    glVertexAttribPointer(mGLData.texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
    glEnableVertexAttribArray(mGLData.texCoordLoc);

    return 1;
}

void init_onscreen()
{
  Init_tex();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// offscreen

int createTBMWindow()
{
#if ENABLE_GL_OFFSCREEN
    mGLData.mTbmDisplay = tbm_bufmgr_init(-1);

    if (!mGLData.mTbmDisplay) {
        printf("[TBM native display] Failed on init .\n");
        return 0;
    } else {
        printf("[TBM native display] Success on init.\n");
    }

    mGLData.mTbmSurfaceQueue = tbm_surface_queue_create(3, SCREEN_WIDTH, SCREEN_HEIGHT, TBM_FORMAT_ARGB8888, 0);
    if (!mGLData.mTbmSurfaceQueue) {
        printf("[TBM native window] Failed on init .\n");
        return 0;
    } else {
        printf("[TBM native window] Success on init.\n");
    }

    mGLData.hWnd = (EGLNativeWindowType)mGLData.mTbmSurfaceQueue;
#else
    tbm_surface_info_s info;
    int w = SCREEN_WIDTH, h = SCREEN_HEIGHT;
    int i, j;

    mGLData.mCurrentSurface = tbm_surface_create(SCREEN_WIDTH, SCREEN_HEIGHT, TBM_FORMAT_ARGB8888);
    //fprintf(stderr,"%s : create surface %p\n",__FUNCTION__,mGLData.mCurrentSurface);
    //fprintf(stderr,"%s : call tbm_surface_map %p\n",__FUNCTION__,mGLData.mCurrentSurface);
    tbm_surface_map(mGLData.mCurrentSurface, TBM_SURF_OPTION_WRITE|TBM_SURF_OPTION_READ, &info);
    unsigned char *ptr = info.planes[0].ptr;
    for (j=0; j<h; j++)
    {
       for (i=0; i<w; i++)
       {
          ptr[0] = 255;
          ptr[1] = 0;
          ptr[2] = 0;
          ptr[3] = 255;
          ptr += 4;
       }
    }

    //fprintf(stderr,"%s : call tbm_surface_unmap %p\n",__FUNCTION__,mGLData.mCurrentSurface);
    tbm_surface_unmap(mGLData.mCurrentSurface);
#endif
    return 1;
}

int createOffscreenEGLContext()
{
    EGLint attribList[] =
    {
        EGL_RED_SIZE,       5,
        EGL_GREEN_SIZE,     6,
        EGL_BLUE_SIZE,      5,
        EGL_ALPHA_SIZE,     8,
        EGL_DEPTH_SIZE,     8,
        EGL_STENCIL_SIZE,   8,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_NONE
    };

    EGLint numConfigs;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    EGLConfig config;
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

    EGLint eglErrorResult;

    // Get Display
    display = eglGetDisplay(mGLData.mTbmDisplay);
    eglErrorResult = eglGetError();

    if ( display == EGL_NO_DISPLAY )
    {
       printf("[eglGetDisplay] Failed.");
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglGetDisplay][eglGetError] 0x%04X\n", eglErrorResult);

       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglGetDisplay][eglGetError] 0x%04X\n", eglErrorResult);

    printf("[eglGetDisplay] Success.\n");

    // Initialize EGL
    if ( !eglInitialize(display, &majorVersion, &minorVersion) )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglInitialize][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglInitialize][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglInitialize] Success.\n");

    // Get configs
    if ( !eglGetConfigs(display, NULL, 0, &numConfigs) )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglGetConfigs][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglGetConfigs][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglGetConfigs] Success.\n");

    // Choose config
    if ( !eglChooseConfig(display, attribList, &config, 1, &numConfigs) )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglChooseConfig][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglChooseConfig][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglChooseConfig] Success.\n");

    // Create a surface
    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)mGLData.hWnd, NULL);
    if ( surface == EGL_NO_SURFACE )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglCreateWindowSurface][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglCreateWindowSurface][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglCreateWindowSurface] Success.\n");

    // Create a GL context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
    if ( context == EGL_NO_CONTEXT )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglCreateContext][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglCreateContext][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglCreateContext] Success.\n");

    // Make the context current
    if ( !eglMakeCurrent(display, surface, surface, context) )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglMakeCurrent][eglGetError] 0x%04X\n", eglErrorResult);
       return 0;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS) printf("[eglMakeCurrent][eglGetError] 0x%04X\n", eglErrorResult);
    printf("[eglMakeCurrent] Success.\n");

    mGLData.eglDisplay = display;
    mGLData.eglSurface = surface;
    mGLData.eglContext = context;
    return 1;
}
int initOffscreenShader()
{
    GLbyte vShaderStr[] =
        "attribute vec4 vPosition;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vPosition;  \n"
        "}                            \n";

    GLbyte fShaderStr[] =
        "precision mediump float;\n"\
        "void main()                                  \n"
        "{                                            \n"
        "  gl_FragColor = vec4 ( 0.0, 1.0, 0.0, 1.0 );\n"
        "}                                            \n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    GLenum glErrorResult;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader ( GL_VERTEX_SHADER, (char*)vShaderStr );
    fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, (char*)fShaderStr );

    // Create the program object
    programObject = glCreateProgram ( );

    if ( programObject == 0 ) {
        if ((glErrorResult = glGetError()) != GL_NO_ERROR)
            printf("[glCreateProgram][glGetError] 0x%04X\n", glErrorResult);
        printf("[glCreateProgram] Failed.\n");
        return 0;
    }
    printf("[glCreateProgram] Success.\n");

    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glGetShaderiv][glGetError] 0x%04X\n", glErrorResult);

    glAttachShader ( programObject, vertexShader );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glAttachShader-vertex][glGetError] 0x%04X\n", glErrorResult);
    glAttachShader ( programObject, fragmentShader );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glAttachShader-fragment][glGetError] 0x%04X\n", glErrorResult);

    // Bind vPosition to attribute 0
    glBindAttribLocation ( programObject, 0, "vPosition" );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glBindAttribLocation][glGetError] 0x%04X\n", glErrorResult);

    // Link the program
    glLinkProgram ( programObject );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glLinkProgram][glGetError] 0x%04X\n", glErrorResult);

    // Check the link status
    glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glGetProgramiv][glGetError] 0x%04X\n", glErrorResult);

    if ( !linked )
    {
        GLint infoLen = 0;

        glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );

            glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
            printf ( "[glGetProgramiv] Error linking program:\n%s\n", infoLog );

            free ( infoLog );
        }

        glDeleteProgram ( programObject );
        return 0;
    }

    // Store the program object
    mGLData.offscreen_programObject = programObject;

    glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
    return 1;
}

void init_offscreen()
{
  createTBMWindow();
#ifndef ENABLE_GL_OFFSCREEN
  createOffscreenEGLContext();
  initOffscreenShader();
#endif
}

// pullic Callbacks
// intialize callback that gets called once for intialization
API void initialize_gl()
{
   printf("init_gl start~~~~\n");

   mGLData.mImageExtension = Dali::GlWindowExtensions::ImageExtension::New( mGLWindow );
   mGLData.mSyncExtension = Dali::GlWindowExtensions::SyncExtension::New( mGLWindow );

   mGLData.IsCreatedEGLImage = false;
   mGLData.IsCreatedSyncObject = false;

   init_onscreen();

   init_offscreen();

   mGLData.mCount = 0;
}

void Draw_offscreen()
{
    //printf("Draw_offscreen\n");
#if ENABLE_GL_OFFSCREEN
    EGLint eglErrorResult;
    if ( !eglMakeCurrent(mGLData.eglDisplay, mGLData.eglSurface, mGLData.eglSurface, mGLData.eglContext) )
    {
       if ((eglErrorResult = eglGetError()) != EGL_SUCCESS)
           printf("[eglMakeCurrent][eglGetError] 0x%04X\n", eglErrorResult);
       return;
    }

    if ((eglErrorResult = eglGetError()) != EGL_SUCCESS)
    {
        printf("[eglMakeCurrent][eglGetError] 0x%04X\n", eglErrorResult);
        return;
    }
    printf("[eglMakeCurrent] Success.\n");

    GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f };

    GLenum glErrorResult;

    // Set the viewport
    glViewport ( 0, 0, 100, 100 );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glViewport][glGetError] 0x%04X\n", glErrorResult);

    // Clear the color buffer
    glClear ( GL_COLOR_BUFFER_BIT );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glClear][glGetError][%d] 0x%04X\n", __LINE__, glErrorResult);

    // Use the program object
    glUseProgram ( mGLData.offscreen_programObject );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glUseProgram][glGetError] 0x%04X\n", glErrorResult);

    // Load the vertex data
    glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glVertexAttribPointer][glGetError] 0x%04X\n", glErrorResult);

    glEnableVertexAttribArray ( 0 );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glEnableVertexAttribArray][glGetError] 0x%04X\n", glErrorResult);

    glDrawArrays ( GL_TRIANGLES, 0, 3 );
    if ((glErrorResult = glGetError()) != GL_NO_ERROR)
        printf("[glDrawArrays][glGetError] 0x%04X\n", glErrorResult);

    eglSwapBuffers( mGLData.eglDisplay, mGLData.eglSurface );
     printf("Draw_offscreen is finished\n");
#else
    tbm_surface_info_s info;
    int i, j;

    //fprintf(stderr,"%s : call tbm_surface_map %p\n",__FUNCTION__,mGLData.mCurrentSurface);
    tbm_surface_map(mGLData.mCurrentSurface, TBM_SURF_OPTION_WRITE|TBM_SURF_OPTION_READ, &info);
    unsigned char *ptr = info.planes[0].ptr;
    unsigned int mark = mGLData.mCount%3;
    //printf("mark : %d\n", mark);
    for (j=0; j<SCREEN_HEIGHT; j++)
    {
       for (i=0; i<SCREEN_WIDTH; i++)
       {
           ptr[0] = 255;
           ptr[1] = 0;
           ptr[2] = 0;
           ptr[3] = 255;
           ptr[mark] = 255;
           ptr += 4;
       }
    }
    //fprintf(stderr,"%s : call tbm_surface_unmap %p\n",__FUNCTION__,mGLData.mCurrentSurface);
    tbm_surface_unmap(mGLData.mCurrentSurface);
#endif
}

void Draw_onscreen()
{
  //printf("Draw_onscreen\n");
  tbm_surface_h tbm_surface = nullptr;
#if ENABLE_GL_OFFSCREEN
  if (tbm_surface_queue_can_acquire((tbm_surface_queue_h)mGLData.hWnd, 1)) {
      tbm_surface_queue_acquire((tbm_surface_queue_h)mGLData.hWnd, &tbm_surface);
      printf("get tbm_surface_queue_acquire: %p\n",tbm_surface);
  }

  if(!tbm_surface)
  {
    printf("current tbm surface is null\n");
    return;
  }

  mGLData.mCurrentSurface = tbm_surface;
  /* Onscreen render */
  mGLWindow.MakeCurrent();
  printf("GLWindow Make Current\n");
#else
  tbm_surface = mGLData.mCurrentSurface;
#endif

  GLenum glErrorResult;
  matrix_t modelview;

  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glClearColor(0xff, 0xff, 0xff, 0xff);
  glClear ( GL_COLOR_BUFFER_BIT );


  if( mGLData.IsCreatedEGLImage == false )
  {
      printf("try to create EGLImage\n");
      if( mGLData.mImageExtension.CreateImageKHR(SCREEN_WIDTH, SCREEN_HEIGHT, NativeImageSource::ColorDepth::COLOR_DEPTH_32,tbm_surface ) )
      {
          mGLData.IsCreatedEGLImage = true;
          printf("success to create EGLImage\n");
      }
      else
      {
          mGLData.IsCreatedEGLImage = false;
          printf("fail to create EGLImage\n");
      }
  }

  //printf("Create EGLImage\n");

  glClear ( GL_COLOR_BUFFER_BIT );
  if ((glErrorResult = glGetError()) != GL_NO_ERROR)
      printf("[glClear][glGetError][%d] 0x%04X\n", __LINE__, glErrorResult);


  // Store the program object

  glBindTexture(GL_TEXTURE_2D, mGLData.textureId);
  //printf("Bind Texture : %d\n", mGLData.textureId);

  printf("Success to wait sync object\n");
  mGLData.mImageExtension.TargetTextureKHR();
  //printf("Create EGLImage\n");


  _matrix_load_identity(&modelview);
  _matrix_translate(&modelview, 0.0f, 0.0f, 0.0f);
  _matrix_multiply(&mGLData.mvp_matrix, &modelview, &mGLData.perspective);

  glUniformMatrix4fv(mGLData.mvpLoc, 1, GL_FALSE, (GLfloat *)&mGLData.mvp_matrix.m[0][0]);
  glUniform1i(mGLData.samplerLoc, 0);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindTexture(GL_TEXTURE_2D, (GLuint)NULL);

  //printf("Draw_onscreen is finished\n");
}

// draw callback is where all the main GL rendering happens
API int renderFrame_gl()
{
#if ENABLE_GL_OFFSCREEN
   if( mGLData.mCount > 0 && mGLData.mCurrentSurface )
   {
     tbm_surface_queue_release((tbm_surface_queue_h)mGLData.hWnd, mGLData.mCurrentSurface);
     mGLData.mCurrentSurface = nullptr;
   }
#endif

   if(mGLData.IsCreatedSyncObject)
   {
       if(mGLData.mSyncExtension.IsSynced())
       {
         printf("IsSynced's true\n");
       }
       else
       {
         printf("IsSynced's false\n");
       }

       mGLData.mSyncExtension.DestroySyncObject();
       mGLData.IsCreatedSyncObject = false;
   }

   Draw_offscreen();

   Draw_onscreen();

   if(mGLData.IsCreatedSyncObject ==  false)
   {
       if( !mGLData.mSyncExtension.CreateSyncObject() )
       {
         printf("Fail to create sync object\n");
       }
       else
       {
         printf("success to create sync object\n");
         mGLData.IsCreatedSyncObject = true;
       }
   }


   mGLData.mCount++;

   return 1;

}


// delete callback gets called when glview is deleted
API void terminate_gl()
{

}

API void update_touch_event_state( bool down )
{
  mGLData.mouse_down = down;
}

API void update_touch_position(int x, int y)
{
#if 0
  float dx = 0;
  float dy = 0;
  mGLData.curPoint.x = (float)x;
  mGLData.curPoint.y = (float)y;

  if( mGLData.mouse_down == true )
  {
    dx = mGLData.curPoint.x - mGLData.prevPoint.x;
    dy = mGLData.curPoint.y - mGLData.prevPoint.y;
    mGLData.anglePoint.x += dy;
    mGLData.anglePoint.y += dx;
  }
  mGLData.prevPoint.x = mGLData.curPoint.x;
  mGLData.prevPoint.y = mGLData.curPoint.y;
#endif
}

API void update_window_size(int w, int h)
{
#if 0
  mGLData.width = w;
  mGLData.height = h;
#endif
}

API void update_window_rotation_angle(int angle)
{
//  mGLData.windowAngle = angle;
}


// This example shows how to create and display Hello World! using a simple TextActor
//
class HelloWorldController : public ConnectionTracker
{
public:

  HelloWorldController( Application& application )
  : mApplication( application )
  {
    // Connect to the Application's Init signal
    mApplication.InitSignal().Connect( this, &HelloWorldController::Create );
  }

  ~HelloWorldController()
  {
    // Nothing to do here;
  }

  // The Init signal is received once (only) during the Application lifetime
  void Create( Application& application )
  {
    mGLData.initialized = false;
    mGLWindow = Dali::GlWindow::New(PositionSize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),"GLWindowSample","",false);
    mGLWindow.SetEglConfig( true, true, 0, Dali::GlWindow::GlesVersion::VERSION_2_0 );
    mGLWindow.RegisterGlCallback( Dali::MakeCallback(initialize_gl),
                                  Dali::MakeCallback(renderFrame_gl),
                                  Dali::MakeCallback(terminate_gl) );
    //mGLWindow.RegisterGlCallback( initialize_gl, renderFrame_gl, terminate_gl );

    mGLData.width = SCREEN_WIDTH;
    mGLData.height = SCREEN_HEIGHT;

    //currentWindowOrientation = Dali::WindowOrientation::NO_ORIENTATION_PREFERENCE;

    mGLWindow.Show();

    mGLWindow.ResizeSignal().Connect( this,  &HelloWorldController::OnGLWindowResize );

    //Dali::Vector<Dali::WindowOrientation> glWindowOrientations;
    //glWindowOrientations.PushBack( Dali::WindowOrientation::PORTRAIT );
    //glWindowOrientations.PushBack( Dali::WindowOrientation::LANDSCAPE );
    //glWindowOrientations.PushBack( Dali::WindowOrientation::PORTRAIT_INVERSE );
    //glWindowOrientations.PushBack( Dali::WindowOrientation::LANDSCAPE_INVERSE );
    //mGLWindow.SetAvailableOrientations( glWindowOrientations );
    mGLWindow.TouchedSignal().Connect( this, &HelloWorldController::OnGLWindowTouch );
    mGLWindow.KeyEventSignal().Connect( this, &HelloWorldController::OnGLWindowKeyEvent );
  }

  void OnWindowResize( Dali::Window winHandle, Dali::Window::WindowSize size )
  {
    int width = size.GetWidth();
    int height = size.GetHeight();
    mTextLabel.SetProperty( Actor::Property::SIZE, Vector2(width, 200) );
    //view_set_ortho( mGLData.view, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
    DALI_LOG_ERROR("OnWindowResized, current orientation:%d, width:%d, height:%d\n", DevelWindow::GetCurrentOrientation(winHandle), width, height );
  }

  bool OnTouch( Actor actor, const TouchEvent& touch )
  {
    // quit the application
    //mUIWindow.Lower();
    return true;
  }

  void OnKeyEvent( const KeyEvent& event )
  {
    if(event.GetState() == KeyEvent::DOWN)
    {
      if ( IsKey( event, Dali::DALI_KEY_ESCAPE ) || IsKey( event, Dali::DALI_KEY_BACK ) )
      {
        //mUIWindow.Raise();
        mApplication.Quit();
      }
    }
  }

  void OnGLWindowResize( Dali::GlWindow::WindowSize size )
  {
#if 1
    int angle = static_cast<int>(mGLWindow.GetCurrentOrientation());
    update_window_rotation_angle( angle );
    update_window_size( size.GetWidth(),  size.GetHeight() );
#else
    currentWindowOrientation = mGLWindow.GetCurrentOrientation();
    mGLData.windowAngle = ( static_cast< int >( currentWindowOrientation ) ) * 90;

    mGLData.width = size.GetWidth();
    mGLData.height = size.GetHeight();
#endif
    DALI_LOG_ERROR("current rotation: %d, width: %d, height: %d\n", mGLData.windowAngle, mGLData.width, mGLData.height );
  }

  void OnGLWindowTouch( const TouchEvent& touch )
  {
     DALI_LOG_ERROR("touch.GetState(), %d\n",touch.GetState( 0 ));
    if( touch.GetState( 0 ) == 0 )
    {
      mGLData.mouse_down = true;
      DALI_LOG_ERROR("DOWN, x:%f, y:%f\n",touch.GetScreenPosition( 0 ).x, touch.GetScreenPosition( 0 ).y);
    }
    else if( touch.GetState( 0 ) == 1 )
    {
      mGLData.mouse_down = false;
      DALI_LOG_ERROR("UP, x:%f, y:%f\n",touch.GetScreenPosition( 0 ).x, touch.GetScreenPosition( 0 ).y);
    }
    else if( touch.GetState( 0 ) == 2 )
    {
      float dx = 0;
      float dy = 0;
      mGLData.curPoint.x = touch.GetScreenPosition( 0 ).x;
      mGLData.curPoint.y = touch.GetScreenPosition( 0 ).y;
      DALI_LOG_ERROR("MOTION x:%f, y:%f\n",mGLData.curPoint.x, mGLData.curPoint.y);
      if( mGLData.mouse_down == true )
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

  void OnGLWindowKeyEvent( const KeyEvent& event )
  {
    if(event.GetState() == KeyEvent::DOWN)
    {
      //if ( IsKey( event, Dali::DALI_KEY_ESCAPE ) || IsKey( event, Dali::DALI_KEY_BACK ) )
      if(event.GetKeyName() == "1")
      {
        Window window = mApplication.GetWindow();
        window.Hide();
        //mApplication.Quit();
        //mUIWindow.Raise();
      }
    }
  }

private:
  Application&    mApplication;
  Dali::Window    mUIWindow;
  TextLabel       mTextLabel;

  //Dali::WindowOrientation currentWindowOrientation;
};

int DALI_EXPORT_API main( int argc, char **argv )
{
  Application application = Application::New( &argc, &argv );
  HelloWorldController test( application );
  application.MainLoop();
  return 0;
}
