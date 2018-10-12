#ifndef _H_VIDEO_RENDER_YUV_
#define _H_VIDEO_RENDER_YUV_

#include "GLUtils.h"

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

class VideoRendererYUV420 {
public:
  VideoRendererYUV420();
  ~VideoRendererYUV420();

  void init(size_t width, size_t height);
  void render(int unityTexture);
  void updateFrame(const video_frame &frame);
  void draw(uint8_t *buffer, size_t length, size_t width, size_t height,
            int rotation);
  bool createTextures();
  bool updateTextures();
  void deleteTextures();
  GLuint createProgram();
  GLuint useProgram();

 protected:
  GLuint m_program;
  GLuint m_vertexShader;
  GLuint m_pixelShader;

  GLuint m_frameBuffer;
  GLuint m_renderTarget;
  int m_unityTextureHandle;

  size_t m_width;
  size_t m_height;
  size_t m_backingWidth;
  size_t m_backingHeight;

  bool isDirty;
  bool isOrientationChanged;

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
  GLint m_uniformProjection;
  GLint m_uniformRotation;
  GLint m_uniformScale;

  int m_rotation;

  void createRenderTarget();
};

#endif // _H_VIDEO_RENDER_YUV_
