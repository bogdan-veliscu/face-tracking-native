#include "VideoRendererYUV420.h"

#include <cstring>

// Vertices for a full screen quad.
static const float kVertices[8] = {-1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f};

// Texture coordinates for mapping entire texture.
static const float kTextureCoords[8] = {0, 0, 0, 1, 1, 0, 1, 1};

// Vertex shader.
static const char kVertexShader[] =
    "#version 100\n"
    "varying vec2 v_texcoord;\n"
    "attribute vec4 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 rotation;\n"
    "uniform mat4 scale;\n"
    "void main() {\n"
    "   v_texcoord = texcoord;\n"
    "   gl_Position = projection * rotation * scale * position;\n"
    "}\n";

// YUV420 to RGB conversion, pixel shader.
static const char kFragmentShader[] =
    "#version 100\n"
    "precision highp float;"
    "varying vec2 v_texcoord;\n"
    "uniform lowp sampler2D s_textureY;\n"
    "uniform lowp sampler2D s_textureU;\n"
    "uniform lowp sampler2D s_textureV;\n"
    "void main() {\n"
    "   float y, u, v, r, g, b;\n"
    "   y = texture2D(s_textureY, v_texcoord).r;\n"
    "   v = texture2D(s_textureU, v_texcoord).a - 0.5;                    \n"
    "   u = texture2D(s_textureU, v_texcoord).r - 0.5;                    \n"

    // The numbers are just YUV to RGB conversion constants
    "   r = y + 1.13983*v;                                      \n"
    "   g = y - 0.39465*u - 0.58060*v;                          \n"
    "   b = y + 2.03211*u;                                      \n"

    "   gl_FragColor = vec4(r, g, b, 1.0);\n"
    "}\n";

VideoRendererYUV420::VideoRendererYUV420()
    : m_pDataY(nullptr), m_pDataUV(nullptr), m_length(0), m_sizeY(0),
      m_sizeUV(0), m_textureIdY(0), m_textureIdUV(0), m_vertexPos(0),
      m_textureLoc(0), m_textureYLoc(0), m_textureUVLoc(0),
      m_uniformProjection(0), m_uniformRotation(0), m_uniformScale(0),
      m_rotation(0), m_program(0), m_vertexShader(0), m_pixelShader(0),
      m_width(0), m_height(0), m_backingWidth(0), m_backingHeight(0),
      isDirty(false), isOrientationChanged(false)

{
  isOrientationChanged = true;
}

VideoRendererYUV420::~VideoRendererYUV420() {
  deleteTextures();

  delete[] m_pDataY;

  if (m_vertexShader) {
    glDetachShader(m_program, m_vertexShader);
    glDeleteShader(m_vertexShader);
    m_vertexShader = 0;
  }
  if (m_pixelShader) {
    glDetachShader(m_program, m_pixelShader);
    glDeleteShader(m_pixelShader);
    m_pixelShader = 0;
  }
  if (m_program) {
    glDeleteProgram(m_program);
    m_program = 0;
  }
}

void VideoRendererYUV420::init(size_t width, size_t height) {
  m_backingWidth = width;
  m_backingHeight = height;
}

void VideoRendererYUV420::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  if (!updateTextures() || !useProgram())
    return;

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  check_gl_error("render");
}

void VideoRendererYUV420::updateFrame(const video_frame &frame) {
  m_sizeY = frame.width * frame.height;
  m_sizeUV = frame.width * frame.height / 2;

  if (m_pDataY == nullptr || m_width != frame.width ||
      m_height != frame.height) {
    m_pDataY = new uint8_t[(m_sizeY + m_sizeUV)];
    m_pDataUV = m_pDataY + m_sizeY;
  }

  m_width = frame.width;
  m_height = frame.height;

  if (m_width == frame.stride_y) {
    memcpy(m_pDataY, frame.y, m_sizeY);
  } else {
    uint8_t *pSrcY = frame.y;
    uint8_t *pDstY = m_pDataY;

    for (int h = 0; h < m_height; h++) {
      memcpy(pDstY, pSrcY, m_width);

      pSrcY += frame.stride_y;
      pDstY += m_width;
    }
  }

  if (m_width / 2 == frame.stride_uv) {
    memcpy(m_pDataUV, frame.uv, m_sizeUV);
  } else {
    uint8_t *pSrcUV = frame.uv;
    uint8_t *pDstUV = m_pDataUV;

    for (int h = 0; h < m_height / 2; h++) {
      memcpy(pDstUV, pSrcUV, m_width / 2);

      pDstUV += m_width / 2;

      pSrcUV += frame.stride_uv;
    }
  }

  LOGI("#### VideoRendererYUV420 ->  frame updated : %d x %d -- [33] = %d" , frame.width, frame.height, frame.y[33]);
  isDirty = true;
}

void VideoRendererYUV420::draw(uint8_t *buffer, size_t length, size_t width,
                               size_t height, int rotation) {
  m_length = length;
  m_rotation = rotation;

  video_frame frame;
  frame.width = width;
  frame.height = height;
  frame.stride_y = width;
  frame.stride_uv = width / 2;
  frame.y = buffer;
  frame.uv = buffer + width * height;

  updateFrame(frame);
}

bool VideoRendererYUV420::createTextures() {
  GLsizei widthY = (GLsizei)m_width;
  GLsizei heightY = (GLsizei)m_height;

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &m_textureIdY);
  glBindTexture(GL_TEXTURE_2D, m_textureIdY);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widthY, heightY, 0, GL_LUMINANCE,
               GL_UNSIGNED_BYTE, NULL);

  if (!m_textureIdY) {
    check_gl_error("Create Y texture");
    return false;
  }

  GLsizei widthU = (GLsizei)m_width / 2;
  GLsizei heightU = (GLsizei)m_height / 2;

  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &m_textureIdUV);
  glBindTexture(GL_TEXTURE_2D, m_textureIdUV);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, widthU, heightU, 0,
               GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);

  if (!m_textureIdUV) {
    check_gl_error("Create U texture");
    return false;
  }
  check_gl_error("");
  LOGI("#### VideoRendererYUV420 ->  Created texture completed!");
  return true;
}

bool VideoRendererYUV420::updateTextures() {
  if (!m_textureIdY && !m_textureIdUV && !createTextures())
    return false;

  if (isDirty) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIdY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei)m_width,
                 (GLsizei)m_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 m_pDataY);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureIdUV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, (GLsizei)m_width / 2,
                 (GLsizei)m_height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                 m_pDataUV);

    isDirty = false;

    LOGI("#### VideoRendererYUV420 ->  updateTextures completed!");
    return true;
  }

  return false;
}

void VideoRendererYUV420::deleteTextures() {
  if (m_textureIdY) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &m_textureIdY);

    m_textureIdY = 0;
  }

  if (m_textureIdUV) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &m_textureIdUV);

    m_textureIdUV = 0;
  }

  LOGI("## VideoRendererYUV420 -> deleteTextures completed!");
}

GLuint VideoRendererYUV420::createProgram() {
  m_program = ::create_program(kVertexShader, kFragmentShader, m_vertexShader,
                               m_pixelShader);

  if (!m_program) {
    check_gl_error("Create program");
    LOGE("Could not create program.");
    return 0;
  }

  m_vertexPos = (GLuint)glGetAttribLocation(m_program, "position");
  m_uniformProjection = glGetUniformLocation(m_program, "projection");
  m_uniformRotation = glGetUniformLocation(m_program, "rotation");
  m_uniformScale = glGetUniformLocation(m_program, "scale");
  m_textureYLoc = glGetUniformLocation(m_program, "s_textureY");
  m_textureUVLoc = glGetUniformLocation(m_program, "s_textureU");
  m_textureLoc = (GLuint)glGetAttribLocation(m_program, "texcoord");

  return m_program;
}

GLuint VideoRendererYUV420::useProgram() {
  if (!m_program && !createProgram()) {
    LOGE("Could not use program.");
    return 0;
  }

  if (isOrientationChanged) {
    glUseProgram(m_program);

    check_gl_error("Use program.");

    glVertexAttribPointer(m_vertexPos, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
    glEnableVertexAttribArray(m_vertexPos);

    float targetAspectRatio = (float)m_height / (float)m_width;

    GLfloat projection[16];
    mat4f_load_ortho(-targetAspectRatio, targetAspectRatio, -1.0f, 1.0f, -1.0f,
                     1.0f, projection);
    glUniformMatrix4fv(m_uniformProjection, 1, GL_FALSE, projection);

    GLfloat rotationZ[16];
    mat4f_load_rotation_z(m_rotation, rotationZ);
    glUniformMatrix4fv(m_uniformRotation, 1, 0, &rotationZ[0]);

    float scaleFactor = aspect_ratio_correction(
        true, m_backingWidth, m_backingHeight, m_width, m_height);

    GLfloat scale[16];
    mat4f_load_scale(scaleFactor, -scaleFactor, 1.0f, scale);
    glUniformMatrix4fv(m_uniformScale, 1, 0, &scale[0]);

    glUniform1i(m_textureYLoc, 0);
    glUniform1i(m_textureUVLoc, 1);
    glVertexAttribPointer(m_textureLoc, 2, GL_FLOAT, GL_FALSE, 0,
                          kTextureCoords);
    glEnableVertexAttribArray(m_textureLoc);

    isOrientationChanged = false;
  }
  LOGI("#### VideoRendererYUV420 ->  useProgram completed!");
  return m_program;
}
