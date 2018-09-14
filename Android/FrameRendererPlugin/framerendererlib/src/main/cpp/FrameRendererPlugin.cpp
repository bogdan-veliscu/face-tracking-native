//
// Created by Bogdan on 10/09/2018.
//

#include "FrameRendererPlugin.h"

#include <jni.h>


static VideoRendererYUV420 *g_renderer = NULL;

static int g_TextureId = 67;

extern "C" {

JNIEXPORT void JNICALL Java_ro_bgx_framerendererlib_FrameRendererPlugin_setScannerEnabled(
    JNIEnv *env, jobject obj, jint enabled);

JNIEXPORT void JNICALL Java_ro_bgx_framerendererlib_FrameRendererPlugin_draw(
    JNIEnv *env, jobject obj, jbyteArray data, jint width, jint height, jint orientation);

};

void _bindTexture(int texID, int width, int height) {
  // binds a texture with the given native hardware texture id through opengl
  if (texID != -1 && g_TextureId != texID) {
    LOGI("# _bindTexture  .. %d to screen : %d x %d", texID, width, height);
    g_TextureId = texID;
    // just update the texture pointer - the drawing is now done on the render
    // event callback

    if (g_renderer == 0) {
      LOGI("# Frame Renderer [_bindTexture] : not yet initialized -- initializing now ..");
      g_renderer = new VideoRendererYUV420();
    }
    g_renderer->init(width, height);
  }
}

JNIEXPORT void JNICALL Java_ro_bgx_framerendererlib_FrameRendererPlugin_draw(
    JNIEnv *env, jobject obj, jbyteArray data, jint width, jint height, jint orientation) {
  LOGI("Java_ro_bgx_framerendererlib_FrameRendererPlugin_draw --> ");
  jbyte *bufferPtr = env->GetByteArrayElements(data, 0);

  jsize arrayLength = env->GetArrayLength(data);

  if (g_renderer != nullptr) {
    g_renderer->draw((uint8_t *) bufferPtr, (size_t) arrayLength, width,
                     height, orientation);
  } else {
    LOGI("Java_com_android_gles3jni_FrameRendererLib_draw --> FrameRenderer is NULL");
  }

  env->ReleaseByteArrayElements(data, bufferPtr, 0);
}

JNIEXPORT void JNICALL Java_ro_bgx_framerendererlib_FrameRendererPlugin_setScannerEnabled(
    JNIEnv *env, jobject obj, jint enabled){
  LOGI("### Scanner callback time: - just returned from JAVA2");
}


// --------------------------------------------------------------------------
// UnitySetInterfaces

/**
 * Simple timer function
 */
long getTimestamp() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return (long) ((now.tv_sec * 1000000000LL + now.tv_nsec) / 1000000LL);
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID) {
  LOGI("# OnRenderEvent ..");

  // binds a texture with the given native hardware texture id through opengl
  if (g_TextureId != -1) {

    if (g_renderer == 0) {
      LOGI("# Frame Renderer not yet initialized -- initializing now ..");
      g_renderer = new VideoRendererYUV420();
    }

    // LOGI("# renderer->renderFrame(%p, %d) ", &mFrame, g_TextureId);
    long start = getTimestamp();
    g_renderer->render();

    long end = getTimestamp();

    LOGI("### Frame conversion took : %d ", (int)(end - start));

  } else {
    LOGE("# Unity texture not yet ready");
    // usleep(1000 * 11);
  }

  //pthread_mutex_unlock(&mDisplayFrame->mutex);
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a
// rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc() {
  return OnRenderEvent;
}


void copyToCharArray(JNIEnv *env, jbyteArray array, unsigned char *buffer) {
  int len = env->GetArrayLength(array);

  env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte *>(buffer));
  // LOGI("copyToCharArray [%p]--> %d", &buffer, len);
}

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces *s_UnityInterfaces = NULL;
static IUnityGraphics *s_Graphics = NULL;

static void *g_TextureHandle = NULL;
static int g_TextureWidth = 0;
static int g_TextureHeight = 0;



extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces *unityInterfaces) {

  LOGI("### VisageFaceAnalyser - UnityPluginLoad");
  s_UnityInterfaces = unityInterfaces;
  s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
  s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

  // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  // LOGI("### VisageFaceAnalyser - UnityPluginUnload");
  s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
SetTextureFromUnity(void *textureHandle, int w, int h) {
  // A script calls this at initialization time; just remember the texture
  // pointer here. Will update texture pixels each frame from the plugin
  // rendering event (texture update needs to happen on the rendering thread).
  LOGI("### VisageFaceAnalyser - SetTextureFromUnity");
  g_TextureHandle = textureHandle;
  g_TextureWidth = w;
  g_TextureHeight = h;
  GLuint frame_tex_id = (GLuint) (size_t) (g_TextureHandle);
}
// --------------------------------------------------------------------------
// GraphicsDeviceEvent

static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

void CreateResources();
static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  // LOGI("### VisageFaceAnalyser - OnGraphicsDeviceEvent");
  // Create graphics API implementation upon initialization
  if (eventType == kUnityGfxDeviceEventInitialize) {
    s_DeviceType = s_Graphics->GetRenderer();

    LOGI("### VisageFaceAnalyser - kUnityGfxDeviceEventInitialize");

    LOGI("### Conversion shader initialized!");
  }

  // Cleanup graphics API implementation upon shutdown
  if (eventType == kUnityGfxDeviceEventShutdown) {
    s_DeviceType = kUnityGfxRendererNull;
  }
}
