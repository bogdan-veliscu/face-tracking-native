//
// Created by Bogdan on 10/09/2018.
//

#ifndef FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H
#define FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H
#include <android/log.h>
#define DEBUG 1

#define  LOG_TAG "FRAME_LIB"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#if DEBUG
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...)
#endif

extern "C" {

/** Binds a texture with the given native hardware texture id through OpenGL.
 */
void _bindTexture(int texID, int width, int height);

}

#endif //FRAMERENDERERPLUGIN_FRAMEREDERERPLUGIN_H
