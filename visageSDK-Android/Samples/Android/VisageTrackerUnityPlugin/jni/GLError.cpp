#include "GLError.h"
#include "PlatformBase.h"
#include <iostream>

#if UNITY_IPHONE
#include <OpenGLES/ES2/gl.h>
#elif UNITY_ANDROID || UNITY_WEBGL
#include <GLES2/gl2.h>
#else
#include "GLEW/glew.h"
#endif

#include <android/log.h>
#define LOG_TAG "OGL3"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;

void _check_gl_error(const char *file, int line, const char *msg) {
  GLenum err(glGetError());

  while (err != GL_NO_ERROR) {
    std::string error;

    switch (err) {
    case GL_INVALID_OPERATION:
      error = "INVALID_OPERATION";
      break;
    case GL_INVALID_ENUM:
      error = "INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      error = "INVALID_VALUE";
      break;
    case GL_OUT_OF_MEMORY:
      error = "OUT_OF_MEMORY";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      error = "INVALID_FRAMEBUFFER_OPERATION";
      break;
    }

    //cerr << "GL_" << error.c_str() << " - " << file << ":" << line << endl;

    LOGE("GL_%s - %s:%d -> %s", error.c_str(), file, line, msg);
    err = glGetError();
  }
}