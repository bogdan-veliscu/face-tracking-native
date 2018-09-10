#include "GLUtils.h"

#include <stdlib.h>
#include <cmath>
#include <iostream>

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

GLuint load_shader(GLenum shaderType, const char *pSource)
{
	GLuint shader = glCreateShader(shaderType);
    if (shader)
    {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen)
            {
                char* buf = (char*) malloc((size_t)infoLen);
                if (buf)
                {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint create_program(const char *pVertexSource, const char *pFragmentSource, GLuint &vertexShader,
                      GLuint &pixelShader)
{
    vertexShader = load_shader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) return 0;

    pixelShader = load_shader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) return 0;

    GLuint program = glCreateProgram();
    if (program)
    {
        glAttachShader(program, vertexShader);
        check_gl_error("glAttachShader");
        glAttachShader(program, pixelShader);
        check_gl_error("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE)
        {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength)
            {
                char* buf = (char*) malloc((size_t)bufLength);
                if (buf)
                {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }

            glDetachShader(program, vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;

            glDetachShader(program, pixelShader);
            glDeleteShader(pixelShader);
            pixelShader = 0;

            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void mat4f_load_ortho(float left, float right, float bottom, float top, float near, float far, float* mat4f)
{
/*
  float projectionMatrix[16] = {
      1,0,0,0,
      0,1,0,0,
      0,0,2,0,
      0,0,-1,1,
  };

*/
		mat4f[0] = 1.0f;
    	mat4f[1] = 0.0f;
    	mat4f[2] = 0.0f;
    	mat4f[3] = 0.0f;

    	mat4f[4] = 0.0f;
    	mat4f[5] = 1.0f;
    	mat4f[6] = 0.0f;
    	mat4f[7] = 0.0f;

    	mat4f[8] = 0.0f;
    	mat4f[9] = 0.0f;
    	mat4f[10] = 2.0f;
    	mat4f[11] = 0.0f;

    	mat4f[12] = 0.0f;
    	mat4f[13] = 0.0f;
    	mat4f[14] = -1.0f;
    	mat4f[15] = 1.0f;
}

void mat4f_load_world_matrix(int rotation, float* mat4f){

/*
      cosPhi,-sinPhi,0,0,
      sinPhi,cosPhi,0,0,
      0,0,1,0,
      0,0,finalDepth,1,
      */
    float radians = rotation * (float)M_PI / 180.0f;
    float s = std::sin(radians);
    float c = std::cos(radians);

    mat4f[0] = c;
    mat4f[1] = -s;
    mat4f[2] = 0.0f;
    mat4f[3] = 0.0f;

    mat4f[4] = s;
    mat4f[5] = c;
    mat4f[6] = 0.0f;
    mat4f[7] = 0.0f;

    mat4f[8] = 0.0f;
    mat4f[9] = 0.0f;
    mat4f[10] = 1.0f;
    mat4f[11] = 0.0f;

    mat4f[12] = 0.0f;
    mat4f[13] = 0.0f;
    mat4f[14] = 0.7f;
    mat4f[15] = 1.0f;

}

void mat4f_load_rotation_z(int rotation, float* mat4f)
{
    float radians = rotation * (float)M_PI / 180.0f;
    float s = std::sin(radians);
    float c = std::cos(radians);

    mat4f[0] = c;
    mat4f[1] = -s;
    mat4f[2] = 0.0f;
    mat4f[3] = 0.0f;

    mat4f[4] = s;
    mat4f[5] = c;
    mat4f[6] = 0.0f;
    mat4f[7] = 0.0f;

    mat4f[8] = 0.0f;
    mat4f[9] = 0.0f;
    mat4f[10] = 1.0f;
    mat4f[11] = 0.0f;

    mat4f[12] = 0.0f;
    mat4f[13] = 0.0f;
    mat4f[14] = 0.7f;
    mat4f[15] = 1.0f;
}

void mat4f_load_scale(float scaleX, float scaleY, float scaleZ, float* mat4f)
{
    mat4f[0] = scaleX;
    mat4f[1] = 0.0f;
    mat4f[2] = 0.0f;
    mat4f[3] = 0.0f;

    mat4f[4] = 0.0f;
    mat4f[5] = scaleY;
    mat4f[6] = 0.0f;
    mat4f[7] = 0.0f;

    mat4f[8] = 0.0f;
    mat4f[9] = 0.0f;
    mat4f[10] = scaleZ;
    mat4f[11] = 0.0f;

    mat4f[12] = 0.0f;
    mat4f[13] = 0.0f;
    mat4f[14] = 0.0f;
    mat4f[15] = 1.0f;
}

float aspect_ratio_correction(bool fillScreen, size_t backingWidth, size_t backingHeight, size_t width, size_t height)
{
    float backingAspectRatio = (float)backingWidth / (float)backingHeight;
    float targetAspectRatio = (float)width / (float)height;

    float scalingFactor;
    if (fillScreen)
    {
      if (backingAspectRatio > targetAspectRatio)
      {
        scalingFactor =  (float)height/ (float)backingHeight;
      }
      else
      {
        scalingFactor = (float)width/ (float)backingWidth ;
      }
    }
    else
    {
        if (backingAspectRatio > targetAspectRatio)
        {
            scalingFactor =  (float)backingHeight / (float)height;
        }
        else
        {
            scalingFactor = (float)backingWidth / (float)width;
        }
    }

    return scalingFactor;
}
