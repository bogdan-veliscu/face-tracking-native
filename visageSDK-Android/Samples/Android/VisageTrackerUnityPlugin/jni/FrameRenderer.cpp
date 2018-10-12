#include "FrameRenderer.h"

#include <cstring>

#include <stdint.h>
#include <math.h>

// Vertices for a full screen quad.
static const float kVertices[8] = {-1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f};

// Texture coordinates for mapping entire texture.
static const float kTextureCoords[8] = {0, 0, 0, 1, 1, 0, 1, 1};

// Vertex shader.
static const char kVertexShader[] =
    "#version 300 es\n"
    "layout(location = 0) in vec2 texcoord;\n"
    "layout(location = 1) in vec4 position;\n"
    "out vec2 v_texcoord;\n"
    "uniform mat4 projectionMatrix;\n"
    "uniform mat4 worldMatrix;\n"
    "void main() {\n"
    "   v_texcoord = texcoord;\n"
    "   gl_Position = projectionMatrix * worldMatrix * position;\n"
    "}\n";

// YUV420 to RGB conversion, pixel shader.
static const char kFragmentShader[] =
    "#version 300 es\n"
//    "precision highp float;"
    "in vec2 v_texcoord;\n"
    "uniform lowp sampler2D s_textureY;\n"
    "uniform lowp sampler2D s_textureU;\n"
    "layout(location = 0) out vec4 outColor;                    \n"

    "void main() {\n"
    "   float y, u, v, r, g, b;\n"
    "   y = texture(s_textureY, v_texcoord).r;\n"
    "   v = texture(s_textureU, v_texcoord).a - 0.5;                    \n"
    "   u = texture(s_textureU, v_texcoord).r - 0.5;                    \n"

    // The numbers are just YUV to RGB conversion constants
    "   r = y + 1.13983*v;                                      \n"
    "   g = y - 0.39465*u - 0.58060*v;                          \n"
    "   b = y + 2.03211*u;                                      \n"

    "   outColor = vec4(r, g, b,1);\n"
    "}\n";

// Simple vertex shader source
#define VERTEX_SHADER_SRC(ver, attr, varying)                                  \
  "#version 300 es\n"                                                          \
  "in highp vec3 pos;\n"                                                       \
  "in lowp vec2 texcoord;\n"                                                   \
  "\n"                                                                         \
  "out lowp vec2 v_texcoord;\n"                                                \
  "\n"                                                                         \
  "uniform highp mat4 worldMatrix;\n"                                          \
  "uniform highp mat4 projMatrix;\n"                                           \
  "\n"                                                                         \
  "void main()\n"                                                              \
  "{\n"                                                                        \
  "	gl_Position = (projMatrix * worldMatrix) * vec4(pos,1);\n"                 \
  "	v_texcoord = texcoord;\n"                                                  \
  "}\n"

static const char* kGlesVProgTextGLES3 = VERTEX_SHADER_SRC("#version 300 es\n", "in", "out");

// Simple fragment shader source
#define FRAGMENT_SHADER_SRC(ver, varying, outDecl, outVar)                     \
  "#version 300 es\n"                                                          \
  "out lowp vec4 fragColor;\n"                                                 \
  "in lowp vec4 ocolor;\n"                                                     \
  "\n"                                                                         \
  "void main()\n"                                                              \
  "{\n"                                                                        \
  "	fragColor = ocolor;\n"                                                     \
  "}\n"

static const char* kGlesFShaderTextGLES3 = FRAGMENT_SHADER_SRC("#version 300 es\n", "in", "out lowp vec4 fragColor;\n", "fragColor");

enum VertexInputs
{
  kVertexInputPosition = 0,
  kVertexInputColor = 1
};


FrameRenderer::FrameRenderer()
    : m_pDataY(nullptr), m_pDataUV(nullptr), m_length(0), m_sizeY(0),
      m_sizeUV(0), m_textureIdY(0), m_textureIdUV(0), m_vertexPos(0),
      m_textureLoc(0), m_textureYLoc(0), m_textureUVLoc(0),
      m_uniformProjectionMatrix(0), m_uniformWorldMatrix(0),
      m_rotation(0), m_program(0), m_vertexShader(0), m_pixelShader(0),
      m_width(0), m_height(0), m_backingWidth(0), m_backingHeight(0),
      isDirty(false), isOrientationChanged(false)

{
  isOrientationChanged = true;
}

FrameRenderer::~FrameRenderer() {
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

void FrameRenderer::init(size_t width, size_t height, int targetTexture) {
  m_backingWidth = width;
  m_backingHeight = height;

  m_unityTexture = targetTexture;

  LOGE("# FrameRenderer::init : %d x %d", m_backingWidth, m_backingHeight);
}

uint8_t *FrameRenderer::render() {

  if (!m_unityTexture || !updateTextures() || !useProgram()){

    LOGE("$$$$ ERROR: m_unityTexture = %d ", m_unityTexture);
    return 0;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

  check_gl_error("m_frameBuffer");
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  check_gl_error("glDrawArrays");

    //drawTriangle();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  check_gl_error("GL_DRAW_FRAMEBUFFER");

  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_frameBuffer);
  check_gl_error("GL_READ_FRAMEBUFFER");

  glReadBuffer(GL_COLOR_ATTACHMENT0);

  check_gl_error("glReadBuffer(m_frameBuffer)");

 glBlitFramebuffer(0, 0, m_backingWidth, m_backingHeight, 0, 0, m_backingWidth,
                   m_backingHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);


  check_gl_error("glBlitFramebuffer");

  uint8_t *data = new uint8_t[m_backingWidth * m_backingHeight * 3];

  glReadPixels(0,0, m_backingWidth, m_backingHeight, GL_RGB,GL_UNSIGNED_BYTE,
   data);

  check_gl_error("glReadPixels");



  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  check_gl_error("glBindFramebuffer 0");

  glBindTexture(GL_TEXTURE_2D, m_unityTexture);
  check_gl_error("glBindTexture ");

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_backingWidth, m_backingHeight,
                  GL_RGB, GL_UNSIGNED_BYTE, data);

  check_gl_error("glTexSubImage2D m_unityTexture");

  LOGE("FrameRenderer::renderFrame[%d x %d] return data ADDR : %p",
       m_backingWidth, m_backingHeight, &data);

  delete[] data;


  return 0;
}

void FrameRenderer::updateFrame(const video_frame &frame) {
  // LOGE("# FrameRenderer::updateFrame : %d x %d", frame.width ,frame.height);

  m_sizeY = frame.width * frame.height;
  m_sizeUV = frame.width * frame.height / 2;

  if (m_pDataY == nullptr || m_width != frame.width ||
      m_height != frame.height) {

    LOGE("$ Capturing frames at size %d x %d", frame.width, frame.height);
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

  LOGE("# FrameRenderer::updateFrame : %d x %d", frame.width ,frame.height);
  isDirty = true;
}

void FrameRenderer::draw(uint8_t *buffer, size_t length, size_t width,
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

bool FrameRenderer::createTextures() {

  // TODO update the init method to actually set the correct width x height
  // before reaching this point

  if (m_width == 0) {
    LOGE("# FrameRenderer::createTextures m_width : %d x % d", m_width,
         m_height);
    return false;
  }

  GLsizei widthY = (GLsizei)m_width;
  GLsizei heightY = (GLsizei)m_height;

  int count = 1;
  ///////////
  check_gl_error("Create Y texture");
  glGenFramebuffers(1, &m_frameBuffer);
  check_gl_error("Create Y texture");
  glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
  //////////
  check_gl_error("Create Y texture");
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &m_textureIdY);
  check_gl_error("Create Y texture");

  glBindTexture(GL_TEXTURE_2D, m_textureIdY);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  check_gl_error("Create Y texture BEFORE");

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widthY, heightY, 0, GL_LUMINANCE,
               GL_UNSIGNED_BYTE, NULL);
  check_gl_error("Create Y texture AFTER");

  if (!m_textureIdY) {
    check_gl_error("Create Y texture");
    return false;
  }

  GLsizei widthU = (GLsizei)m_width / 2;
  GLsizei heightU = (GLsizei)m_height / 2;

  glActiveTexture(GL_TEXTURE1);

  check_gl_error("glTexImage2D GL_TEXTURE1");
  glGenTextures(1, &m_textureIdUV);

  check_gl_error("glTexImage2D GL_TEXTURE1");
  glBindTexture(GL_TEXTURE_2D, m_textureIdUV);

  check_gl_error("glTexImage2D GL_TEXTURE1");

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  check_gl_error("glTexImage2D BEFORE");
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, widthU, heightU, 0,
               GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);

  check_gl_error("glTexImage2D AFTER");

  if (!m_textureIdUV) {
    check_gl_error("Create U texture");
    return false;
  }

  glActiveTexture(GL_TEXTURE2);
  check_gl_error("glActiveTexture GL_TEXTURE2");
  // The texture we're going to render to
  glGenTextures(1, &m_renderTarget);

  // "Bind" the newly created texture : all future texture functions will modify
  // this texture
  glBindTexture(GL_TEXTURE_2D, m_renderTarget);
  check_gl_error("glBindTexture renderTarget");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  check_gl_error("glTexParameteri");

  LOGE("@ Trying to load glTexSubImage2D(%d) for screen : %d x %d using "
       "preview : %d x %d",
       m_renderTarget, m_backingWidth, m_backingHeight, widthY, heightY);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_backingWidth, m_backingHeight, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

  check_gl_error("glTexImage2D -> render target33");

  glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
  check_gl_error("init");

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_renderTarget, 0);
  check_gl_error("glFramebufferTexture");

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

  check_gl_error("init");
  // Always check that our framebuffer is ok
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOGE("# not GL_FRAMEBUFFER_COMPLETE : %#x",
         glCheckFramebufferStatus(GL_FRAMEBUFFER));
  } else {
    LOGE("# GL_FRAMEBUFFER_COMPLETE : %#x",
         glCheckFramebufferStatus(GL_FRAMEBUFFER));
  }

  return true;
}

bool FrameRenderer::updateTextures() {
  if (!m_textureIdY && !m_textureIdUV && !createTextures()){

    LOGE("# FrameRenderer::updateTextures ERROR : m_textureIdY : %d , m_textureIdUV : %d",m_textureIdY, m_textureIdUV);
    return false;
  }
  if (isDirty) {

    // LOGE("# FrameRenderer::updateTextures isDirty : y: %p  uv: %p", m_pDataY
    // , m_pDataUV);
    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    check_gl_error("updateTextures");
    glActiveTexture(GL_TEXTURE0);

    check_gl_error("updateTextures");
    glBindTexture(GL_TEXTURE_2D, m_textureIdY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei)m_width,
                 (GLsizei)m_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 m_pDataY);

    check_gl_error("updateTextures");

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureIdUV);

    check_gl_error("updateTextures");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, (GLsizei)m_width / 2,
                 (GLsizei)m_height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                 m_pDataUV);

    check_gl_error("updateTextures");
    isDirty = false;

    return true;
  }

  LOGI("# FrameRenderer::updateTextures not DIRTY ERROR : m_textureIdY : %d , m_textureIdUV : %d",m_textureIdY, m_textureIdUV);
  return false;
}

void FrameRenderer::deleteTextures() {
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
}


void FrameRenderer::DrawSimpleTriangles(const float worldMatrix[16], int triangleCount, const void* verticesFloat3Byte4)
{
  // Set basic render state
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // Tweak the projection matrix a bit to make it match what identity projection would do in D3D case.
  float projectionMatrix[16] = {
      1,0,0,0,
      0,1,0,0,
      0,0,2,0,
      0,0,-1,1,
  };

  // Setup shader program to use, and the matrices
  glUseProgram(m_program2);
  glUniformMatrix4fv(m_UniformWorldMatrix, 1, GL_FALSE, worldMatrix);
  glUniformMatrix4fv(m_UniformProjMatrix, 1, GL_FALSE, projectionMatrix);

  // Bind a vertex buffer, and update data in it
  const int kVertexSize = 12 + 4;
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
  glBufferSubData(GL_ARRAY_BUFFER, 0, kVertexSize * triangleCount * 3, verticesFloat3Byte4);

  // Setup vertex layout
  glEnableVertexAttribArray(kVertexInputPosition);
  glVertexAttribPointer(kVertexInputPosition, 3, GL_FLOAT, GL_FALSE, kVertexSize, (char*)NULL + 0);
  glEnableVertexAttribArray(kVertexInputColor);
  glVertexAttribPointer(kVertexInputColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, kVertexSize, (char*)NULL + 12);

  // Draw
  glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);

}

void FrameRenderer::drawTriangle(){

  // Draw a colored triangle. Note that colors will come out differently
  // in D3D and OpenGL, for example, since they expect color bytes
  // in different ordering.
  struct MyVertex
  {
    float x, y, z;
    unsigned int color;
  };
  MyVertex verts[3] =
      {
          { -0.5f, -0.25f,  0, 0xFFff0000 },
          { 0.5f, -0.25f,  0, 0xFF00ff00 },
          { 0,     0.5f ,  0, 0xFF0000ff },
      };

  // Transformation matrix: rotate around Z axis based on time.
  float phi = 1; // time set externally from Unity script
  float cosPhi = cosf(phi);
  float sinPhi = sinf(phi);
  float depth = 0.7f;
  float finalDepth = depth;
  float worldMatrix[16] = {
      cosPhi,-sinPhi,0,0,
      sinPhi,cosPhi,0,0,
      0,0,1,0,
      0,0,finalDepth,1,
  };

  DrawSimpleTriangles(worldMatrix, 1, verts);
}

GLuint FrameRenderer::createProgram() {
  m_program = ::create_program(kGlesVProgTextGLES3, kFragmentShader, m_vertexShader,
                               m_pixelShader);

  if (!m_program) {
    check_gl_error("Create program");
    LOGE("Could not create program.");
    return 0;
  }

  m_vertexPos = (GLuint)glGetAttribLocation(m_program, "position");
  m_uniformProjectionMatrix = glGetUniformLocation(m_program, "projectionMatrix");
  m_uniformWorldMatrix  = glGetUniformLocation(m_program, "worldMatrix");
  m_textureYLoc = glGetUniformLocation(m_program, "s_textureY");
  m_textureUVLoc = glGetUniformLocation(m_program, "s_textureU");
  m_textureLoc = (GLuint)glGetAttribLocation(m_program, "texcoord");

/*
m_program2 = ::create_program(kGlesVProgTextGLES3, kGlesFShaderTextGLES3, m_vertexShader2, m_pixelShader2);


  m_UniformWorldMatrix = glGetUniformLocation(m_program2, "worldMatrix");
  m_UniformProjMatrix = glGetUniformLocation(m_program2, "projMatrix");

  glGenBuffers(1, &m_VertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 1024, NULL, GL_STREAM_DRAW);

  glUseProgram(m_program);
  */

  return m_program;
}

GLuint FrameRenderer::useProgram() {
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
    glUniformMatrix4fv(m_uniformProjectionMatrix, 1, GL_FALSE, projection);

    GLfloat worldMatrix[16];
    mat4f_load_world_matrix(m_rotation, worldMatrix);
    glUniformMatrix4fv(m_uniformWorldMatrix, 1, 0, &worldMatrix[0]);

    glUniform1i(m_textureYLoc, 0);
    glUniform1i(m_textureUVLoc, 1);
    glVertexAttribPointer(m_textureLoc, 2, GL_FLOAT, GL_FALSE, 0,
                          kTextureCoords);
    glEnableVertexAttribArray(m_textureLoc);

    check_gl_error("Use program END");
    isOrientationChanged = false;
  }

  return m_program;
}
