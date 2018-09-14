#ifndef _H_GL_UTILS_
#define _H_GL_UTILS_

#include <GLES3/gl3.h>
#include <android/log.h>

#define DEBUG 1

#define  LOG_TAG "FRAME_CPP"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#if DEBUG
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...)
#endif

GLuint load_shader(GLenum shaderType, const char *pSource);
GLuint create_program(const char *pVertexSource, const char *pFragmentSource, GLuint &vertexShader,
                      GLuint &pixelShader);
void check_gl_error2(const char *op);
void mat4f_load_ortho(float left, float right, float bottom, float top, float near, float far, float* mat4f);
void mat4f_load_rotation_z(int rotation, float* mat4f);
void mat4f_load_scale(float scaleX, float scaleY, float scaleZ, float* mat4f);
float aspect_ratio_correction(bool fillScreen, size_t backingWidth, size_t backingHeight, size_t width, size_t height);


void _check_gl_error(const char *file, int line, const char *msg);

///
/// Usage
/// [... some opengl calls]
/// glCheckError();
///
#define check_gl_error(MSG) _check_gl_error(__FILE__, __LINE__, MSG)

#endif // _H_GL_UTILS_
