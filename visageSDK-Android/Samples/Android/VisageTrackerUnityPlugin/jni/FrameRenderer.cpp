#include "FrameRenderer.h"

#include <cstring>

#include <stdint.h>

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
        "uniform mat4 projection;\n"
        "uniform mat4 rotation;\n"
        "uniform mat4 scale;\n"
        "void main() {\n"
        "   v_texcoord = texcoord;\n"
        "   gl_Position = projection * rotation * scale * position;\n"
        "}\n";

// YUV420 to RGB conversion, pixel shader.
static const char kFragmentShader[] =
        "#version 300 es\n"
        "precision highp float;"
        "in vec2 v_texcoord;\n"
        "uniform lowp sampler2D s_textureY;\n"
        "uniform lowp sampler2D s_textureU;\n"
        "layout(location = 0) out vec3 outColor;                    \n"

        "void main() {\n"
        "   float y, u, v, r, g, b;\n"
        "   y = texture(s_textureY, v_texcoord).r;\n"
        "   v = texture(s_textureU, v_texcoord).a - 0.5;                    \n"
        "   u = texture(s_textureU, v_texcoord).r - 0.5;                    \n"

        // The numbers are just YUV to RGB conversion constants
        "   r = y + 1.13983*v;                                      \n"
        "   g = y - 0.39465*u - 0.58060*v;                          \n"
        "   b = y + 2.03211*u;                                      \n"

        "   outColor = vec3(r, g, b);\n"
        "}\n";

FrameRenderer::FrameRenderer()
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

  m_renderTarget = targetTexture;

  LOGE("# FrameRenderer::init : %d x %d", m_backingWidth, m_backingHeight);
}

uint8_t*  FrameRenderer::render() {

  if (!m_renderTarget|| !updateTextures() || !useProgram())
    return 0;

  glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);


  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

  check_gl_error("m_frameBuffer");
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  check_gl_error("glDrawArrays");

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  check_gl_error("GL_DRAW_FRAMEBUFFER");
  //  glBindVertexArray(mVBA_Result);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_frameBuffer);
  check_gl_error("GL_READ_FRAMEBUFFER");

 // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  check_gl_error("GL_DRAW_FRAMEBUFFER");
  //  glBindVertexArray(mVBA_Result);

  //glBindFramebuffer(GL_READ_FRAMEBUFFER, m_frameBuffer);
  check_gl_error("GL_READ_FRAMEBUFFER");

  glReadBuffer(GL_COLOR_ATTACHMENT0);

  glBlitFramebuffer(
      0, 0, m_backingWidth, m_backingHeight,
      0, 0, m_backingWidth, m_backingHeight,
      GL_COLOR_BUFFER_BIT, GL_NEAREST);

  uint8_t * data = new uint8_t [m_backingWidth * m_backingHeight * 3];


  //LOGE("FrameRenderer::renderFrame return data ADDR : %p", &data);
  glReadPixels(0,0, m_width, m_height, GL_RGB,GL_UNSIGNED_BYTE, data);

  for(int i = 0; i< m_backingWidth * m_backingHeight; i++){
    data[i] += 50;
    data[++i] += 20;
  }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

  	glBindTexture(GL_TEXTURE_2D, m_renderTarget);
  	check_gl_error("glBindTexture ");
  	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_backingWidth, m_backingHeight, GL_RGB, GL_UNSIGNED_BYTE, data);
  return data;
}

void FrameRenderer::updateFrame(const video_frame &frame) {
  //LOGE("# FrameRenderer::updateFrame : %d x %d", frame.width ,frame.height);

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

  //LOGE("# FrameRenderer::updateFrame : %d x %d", frame.width ,frame.height);
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

  if (m_width != 1080) {
    LOGE("# FrameRenderer::createTextures m_width : %d x % d", m_width,
         m_height);
    m_width = 1080;
    m_height = 1920;
  }

  GLsizei widthY = (GLsizei)m_width;
  GLsizei heightY = (GLsizei)m_height;

  int count =1;
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
  //glGenTextures(1, &m_renderTarget);

  // "Bind" the newly created texture : all future texture functions will modify
  // this texture
  glBindTexture(GL_TEXTURE_2D, m_renderTarget);
  check_gl_error("glBindTexture renderTarget");


  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  check_gl_error("glTexParameteri");

  LOGE("@ Trying to load glTexSubImage2D(%d) for screen : %d x %d using preview : %d x %d" , m_renderTarget ,m_backingWidth,  m_backingHeight,  widthY, heightY );

  if (m_backingWidth != 1080){
    m_backingWidth = 1080;
    m_backingHeight = 2220;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_backingWidth , m_backingHeight , 0, GL_RGB,
               GL_UNSIGNED_BYTE, 0);
  //delete[] tex;
  check_gl_error("glTexImage2D -> render target");

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
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
    LOGE("# not GL_FRAMEBUFFER_COMPLETE : %#x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
  } else {
    LOGE("# GL_FRAMEBUFFER_COMPLETE : %#x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
  }

  return true;
}

bool FrameRenderer::updateTextures() {
  if (!m_textureIdY && !m_textureIdUV && !createTextures())
    return false;

  if (isDirty) {

    //LOGE("# FrameRenderer::updateTextures isDirty : y: %p  uv: %p", m_pDataY , m_pDataUV);
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

GLuint FrameRenderer::createProgram() {
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

  return m_program;
}
