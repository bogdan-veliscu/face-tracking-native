#ifndef _H_VIDEO_RENDER_YUV_
#define _H_VIDEO_RENDER_YUV_

#include "GLUtils.h"
#include <GLES3/gl3.h>

#include <memory>
#include <stdint.h>

enum { tYUV420 };

struct video_frame {
  size_t width;
  size_t height;
  size_t stride_y;
  size_t stride_uv;
  uint8_t *y;
  uint8_t *uv;
};

class FrameRenderer {
 public:
  FrameRenderer();
  ~FrameRenderer();

  void init(size_t width, size_t height, int texId);
  uint8_t * render();
  void updateFrame(const video_frame &frame);
  void draw(uint8_t *buffer, size_t length, size_t width, size_t height,
			int rotation);
  bool createTextures();
  bool updateTextures();
  void deleteTextures();
  GLuint createProgram();
  GLuint useProgram();

  void drawTriangle();

  void DrawSimpleTriangles(const float worldMatrix[16], int triangleCount, const void* verticesFloat3Byte4);

 protected:
  GLuint m_program;
  GLuint m_vertexShader;
  GLuint m_pixelShader;

/*
  GLuint m_program2;
  GLuint m_vertexShader2;
  GLuint m_pixelShader2;

  GLuint m_UniformWorldMatrix;
  GLuint m_UniformProjMatrix;

  GLuint  m_VertexBuffer;
*/
  size_t m_width;
  size_t m_height;
  size_t m_backingWidth;
  size_t m_backingHeight;

  bool isDirty;
  bool isOrientationChanged;

  GLuint m_frameBuffer = 0 ;
  GLuint m_renderTarget;
  GLuint m_unityTexture;

 private:
  uint8_t *m_pDataY;

  uint8_t *m_pDataUV;

  size_t m_length;
  size_t m_sizeY;
  size_t m_sizeUV;

  GLuint m_textureIdY;
  GLuint m_textureIdUV;

  GLuint m_vertexPos;
  GLuint m_textureLoc;
  GLint m_textureYLoc;
  GLint m_textureUVLoc;
  GLint m_uniformProjectionMatrix;
  GLint m_uniformWorldMatrix;

  int m_rotation;


};

#endif // _H_VIDEO_RENDER_YUV_
