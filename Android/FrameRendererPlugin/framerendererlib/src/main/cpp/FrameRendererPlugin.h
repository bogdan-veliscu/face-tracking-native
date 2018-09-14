//
// Created by Bogdan on 10/09/2018.
//

#ifndef FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H
#define FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H

#include "Unity/IUnityGraphics.h"


#include "VideoRendererYUV420.h"
#include "FrameRendererPlugin.h"
#include "GLUtils.h"

static void printGlString(const char *name, GLenum s) {
  const char *v = (const char *) glGetString(s);
  LOGI("GL %s: %s\n", name, v);
}

extern "C" {

/** Binds a texture with the given native hardware texture id through OpenGL.
 */
void _bindTexture(int texID, int width, int height);

UnityRenderingEvent GetRenderEventFunc();

}

#endif //FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H
