//
//  VisageTrackerUnityPlugin.cpp
//  VisageTrackerUnityPlugin
//
//
#include "VisageTrackerUnityPlugin.h"
#include "AndroidCameraCapture.h"
#include "Common.h"
#include "Unity/IUnityGraphics.h"
#include "VisageFaceAnalyser.h"
#include "VisageTracker.h"
#include "WrapperOpenCV.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <jni.h>
#include <thread>
#include <unistd.h>

#include "GLError.h"
#include "PlatformBase.h"

#include <assert.h>
#if UNITY_IPHONE
#include <OpenGLES/ES2/gl.h>
#elif UNITY_ANDROID || UNITY_WEBGL
#include <GLES2/gl2.h>
#else
#include "GLEW/glew.h"
#endif

#include <android/log.h>
#define LOG_TAG "UnityPlugin v7"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace VisageSDK;

static bool previewStarted = false;

static void *g_TextureHandle = NULL;

static int g_TextureWidth = 0;
static int g_TextureHeight = 0;

static GLuint frame_tex_id = 0;

GLuint m_VertexShader;
GLuint m_FragmentShader;
GLuint m_Program;
GLuint m_FrameBuffer;
GLuint m_RenderBuffer;
GLuint m_VertexBuffer;

int m_UniformWorldMatrix;
int m_UniformProjMatrix;

static const char *kGlesVProgTextGLES3 =
    "attribute vec4 a_position;                         \n"
    "attribute vec2 a_texCoord;                         \n"
    "varying vec2 v_texCoord;                           \n"

    "void main(){                                       \n"
    "   gl_Position = a_position;                       \n"
    "   v_texCoord = a_texCoord;                        \n"
    "}                                                  \n";

#undef VERTEX_SHADER_SRC

static const char *kGlesFShaderTextGLES3 =
    "#ifdef GL_ES                                       \n"
    "precision highp float;                             \n"
    "#endif                                             \n"

    "varying vec2 v_texCoord;                           \n"
    "uniform sampler2D y_texture;                       \n"
    "uniform sampler2D uv_texture;                      \n"

    "void main (void){                                  \n"
    "   float r, g, b, y, u, v;                         \n"

    // We had put the Y values of each pixel to the R,G,B components by
    // GL_LUMINANCE, that's why we're pulling it from the R component, we
    // could also use G or B
    "   y = texture2D(y_texture, v_texCoord).r;         \n"

    // We had put the U and V values of each pixel to the A and R,G,B
    // components of the texture respectively using GL_LUMINANCE_ALPHA.
    // Since U,V bytes are interspread in the texture, this is probably the
    // fastest way to use them in the shader
    "   u = texture2D(uv_texture, v_texCoord).a - 0.5;  \n"
    "   v = texture2D(uv_texture, v_texCoord).r - 0.5;  \n"

    // The numbers are just YUV to RGB conversion constants
    "   r = y + 1.13983*v;                              \n"
    "   g = y - 0.39465*u - 0.58060*v;                  \n"
    "   b = y + 2.03211*u;                              \n"

    // We finally set the RGB color of our pixel
    "   gl_FragColor = vec4(r, g, b, 1.0);              \n"
    "}                                                  \n";

GLfloat vertices[] = {-1, -1, 0,  // bottom left corner
                      -1, 1,  0,  // top left corner
                      1,  1,  0,  // top right corner
                      1,  -1, 0}; // bottom right corner

GLubyte indices[] = {
    0, 1, 2,  // first triangle (bottom left - top left - top right)
    0, 2, 3}; // second triangle (bottom left - top right - bottom right)

const GLuint kStride = 0; // COORDS_PER_VERTEX * 4;
const GLshort kIndicesInformation[] = {0, 1, 2, 0, 2, 3};

static GLuint positionObject;
static GLuint texturePosition;
static GLuint yTextureObject;
static GLuint uvTextureObject;
// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the
// scripts.

static float g_Time;
// static int GL_TEXTURE_EXTERNAL_OES = 0x8D65;
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
SetTimeFromUnity(float t) {
  g_Time = t;
}

// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of
// the scripts.

// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces *s_UnityInterfaces = NULL;
static IUnityGraphics *s_Graphics = NULL;

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
  GLuint frame_tex_id = (GLuint)(size_t)(g_TextureHandle);
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

    CreateResources();

    LOGI("### Conversion shader initialized!");
  }

  // Cleanup graphics API implementation upon shutdown
  if (eventType == kUnityGfxDeviceEventShutdown) {
    s_DeviceType = kUnityGfxRendererNull;
  }
}

static GLuint CreateShader(GLenum type, const char *sourceText) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &sourceText, NULL);
  glCompileShader(shader);

  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
      char *buf = (char *)malloc(infoLen);
      if (buf) {
        glGetShaderInfoLog(shader, infoLen, NULL, buf);
        LOGE("Could not compile shader %d:\n%s\n", type, buf);
        free(buf);
      }
      glDeleteShader(shader);
      shader = 0;
    }
  }
  return shader;
}

enum VertexInputs { kVertexInputPosition = 0, kVertexInputColor = 1 };

void CreateResources() {

  LOGI("### CreateResources API: %d", s_DeviceType);
  // Create shaders
  if (s_DeviceType == kUnityGfxRendererOpenGLES30) {

    LOGI("### CreateResources - kUnityGfxRendererOpenGLES30");
    m_VertexShader = CreateShader(GL_VERTEX_SHADER, kGlesVProgTextGLES3);
    printGLError("GL_VERTEX_SHADER");
    m_FragmentShader = CreateShader(GL_FRAGMENT_SHADER, kGlesFShaderTextGLES3);
    printGLError("GL_FRAGMENT_SHADER");
  }

  // Link shaders into a program and find uniform locations
  m_Program = glCreateProgram();
  printGLError("glCreateProgram");

  glAttachShader(m_Program, m_VertexShader);
  printGLError("m_VertexShader");

  glAttachShader(m_Program, m_FragmentShader);
  printGLError("m_FragmentShader");

  glLinkProgram(m_Program);
  printGLError("glLinkProgram");

  GLint status = 0;
  glGetProgramiv(m_Program, GL_LINK_STATUS, &status);

  printGLError("glGetProgramiv");
  assert(status == GL_TRUE);

  // get index of the generic vertex attribute bound to vPosition
  positionObject = glGetAttribLocation(m_Program, "a_position");
  printGLError("glGetAttribLocation");

  // get index of the generic vertex attribute bound to vTexCoord
  texturePosition = glGetAttribLocation(m_Program, "a_texCoord");
  printGLError("glGetAttribLocation");

  yTextureObject = glGetUniformLocation(m_Program, "y_texture");
  printGLError("glGetUniformLocation");

  uvTextureObject = glGetUniformLocation(m_Program, "uv_texture");
  printGLError("glGetUniformLocation");

  // Create vertex buffer
  glGenBuffers(1, &m_VertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 1024, NULL, GL_STREAM_DRAW);

  assert(glGetError() == GL_NO_ERROR);

  LOGI("CREATED shader program successfully!");

  assert(glGetError() == GL_NO_ERROR);
}

static int g_TextureId = -1;

static callbackFunc scanCallback;

static VisageTracker *m_Tracker = 0;
static VisageFaceAnalyser *m_FaceAnalizer = 0;

static int detectedAge = -2;
static int detectedGender = -2;
static int ageRefreshRequested = 1;

int format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
static VsImage *trackImage = 0;
static VsImage *renderImage = 0;
static VsImage *drawImageBuffer = 0;

static AndroidCameraCapture *imageCapture = 0;

// Helper method to create C string copy
static char *MakeStringCopy(const char *val) {
  if (val == NULL)
    return NULL;

  char *res = (char *)malloc(strlen(val) + 1);
  strcpy(res, val);
  return res;
}

static unsigned int GetNearestPow2(unsigned int num) {
  unsigned int n = num > 0 ? num - 1 : 0;

  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;

  return n;
}

namespace VisageSDK {
void initializeLicenseManager(JNIEnv *env, jobject obj,
                              const char *licenseKeyFileName,
                              void (*alertFunction)(const char *) = 0);
}

// When native code plugin is implemented in .mm / .cpp file, then functions
// should be surrounded with extern "C" block to conform C function naming rules
extern "C" {

static FaceData trackingData[10];
static int *trackingStatus = TRACK_STAT_OFF;
//
static int camWidth = 0;
static int camHeight = 0;

static int trackWidth = 240;
static int trackHeight = 320;

static int camOrientation = 0;
static int camFlip = 0;
static float xTexScale;
static float yTexScale;
//
static bool parametersChanged;

static bool scannerEnabled;
static bool scannerWaitingForFrames;

static bool switchingCamera;

transitionCallback initScannerCallback;
transitionCallback releaseScannerCallback;

pthread_mutex_t writeFrame_mutex;
pthread_mutex_t grabFrame_mutex;
pthread_mutex_t displayFrame_mutex;

JavaVM *_vm = 0;
JNIEnv *jni_env = 0;
jobject obj_Activity;

static int textureID = 0;

void AlertCallback(const char *warningMessage) {
  jclass dataClass = jni_env->GetObjectClass(obj_Activity);
  if (jni_env->ExceptionCheck())
    jni_env->ExceptionClear();
  if (dataClass != NULL) {
    jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
    jmethodID javaMethodRef = jni_env->GetMethodID(
        javaClassRef, "AlertDialogFunction", "(Ljava/lang/String;)V");
    if (jni_env->ExceptionCheck())
      jni_env->ExceptionClear();
    jstring message = jni_env->NewStringUTF(warningMessage);
    if (javaMethodRef != 0) {
      jni_env->CallVoidMethod(obj_Activity, javaMethodRef, message);
    }

    jni_env->DeleteGlobalRef(javaClassRef);
    jni_env->DeleteLocalRef(message);
  }
}

// Example how to obtain JNI environment and activity of this Unity sample
// required for license check.
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  _vm = vm;

  return JNI_VERSION_1_6;
}

void _initTracker(char *configuration, char *license) {
  jni_env = 0;
  _vm->AttachCurrentThread(&jni_env, 0);
  jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
  jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
                                                    "Landroid/app/Activity;");
  obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

  LOGI("_initTracker with license: %s", license);
  initializeLicenseManager(jni_env, obj_Activity, license, AlertCallback);

  if (m_Tracker) {
    delete m_Tracker;
    delete m_FaceAnalizer;
  }
  //_releaseTracker();

  m_Tracker = new VisageTracker(configuration);
  pthread_mutex_init(&writeFrame_mutex, NULL);
  pthread_mutex_init(&grabFrame_mutex, NULL);

  // Set up mutex for render thread synchronization
  pthread_mutex_destroy(&displayFrame_mutex);
  pthread_mutex_init(&displayFrame_mutex, NULL);
}

void _releaseTracker() {
  delete m_Tracker;
  delete m_FaceAnalizer;

  LOGI("@@ QR _releaseTracker v2");
  pthread_mutex_lock(&grabFrame_mutex);

  vsReleaseImage(&drawImageBuffer);
  drawImageBuffer = 0;
  vsReleaseImage(&renderImage);
  renderImage = 0;
  m_Tracker = 0;
  pthread_mutex_unlock(&grabFrame_mutex);
}

void _initFaceAnalyser(char *config, char *license) {
  jni_env = 0;
  _vm->AttachCurrentThread(&jni_env, 0);
  jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
  jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
                                                    "Landroid/app/Activity;");
  obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

  initializeLicenseManager(jni_env, obj_Activity, license, AlertCallback);

  if (m_FaceAnalizer) {
    delete m_FaceAnalizer;
  }
  m_FaceAnalizer = new VisageFaceAnalyser();
  LOGI("@@ VisageFaceAnalyser init with config: %s\n", config);
  int ret = m_FaceAnalizer->init(config);
  LOGI("### VisageFaceAnalyser _initFaceAnalyser :%d", ret);
}

void _refreshAgeEstimate() { ageRefreshRequested = 1; }

int _estimateAge() { return detectedAge; }
int _estimateGender() { return detectedGender; }

void Java_app_specta_inc_camera_CameraActivity_WriteFrame(JNIEnv *env,
                                                          jobject obj,
                                                          jbyteArray frame) {

  pthread_mutex_lock(&writeFrame_mutex);
  if (!parametersChanged) {
    jbyte *pixelData = env->GetByteArrayElements(frame, 0);
    imageCapture->WriteFrameYUV((unsigned char *)pixelData);
    env->ReleaseByteArrayElements(frame, pixelData, 0);
  } else {
    LOGI("Write frame after changing parameters");
    switchingCamera = false;
  }

  parametersChanged = false;
  // LOGI("Java_app_specta_inc_camera_CameraActivity_WriteFrame - END");
  pthread_mutex_unlock(&writeFrame_mutex);

  if (scannerWaitingForFrames && scannerEnabled == false) {
    scannerEnabled = true;
    LOGI("Java_app_specta_inc_camera_CameraActivity_init - START");
  }
}

void Java_app_specta_inc_camera_CameraActivity_setScannerEnabled(JNIEnv *env,
                                                                 jobject obj,
                                                                 jint enabled) {
  LOGI("### Scanner callback time: - just returned from JAVA2");

  scannerWaitingForFrames = enabled != 0;

  scannerEnabled = false;
  if (scannerWaitingForFrames) {
    if (initScannerCallback != NULL) {
      initScannerCallback();
    }
  } else {
    if (releaseScannerCallback != NULL) {
      releaseScannerCallback();
    }
  }
}
void Java_app_specta_inc_camera_CameraActivity_setParameters(
    JNIEnv *env, jobject obj, jint orientation, jint width, jint height,
    jint flip) {
  pthread_mutex_lock(&grabFrame_mutex);
  pthread_mutex_lock(&writeFrame_mutex);
  if (width != -1)
    camWidth = width;
  if (height != -1)
    camHeight = height;
  if (orientation != -1) {
    camOrientation = orientation;
    // LOGI("#### setParameters: %d ", orientation);
  }
  if (flip != -1)
    camFlip = flip;

  float factor = 1;
  if (camWidth > height) {
    factor = (float)320 / camWidth;
  } else {
    factor = (float)320 / camHeight;
  }

  trackWidth = camWidth;
  trackHeight = camHeight;

  LOGI("### sett params : tracking %d:%d & preview %d:%d", trackWidth,
       trackHeight, camWidth, camHeight);

  delete imageCapture;
  imageCapture =
      new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);

  if (camOrientation == 90 || camOrientation == 270) {
    xTexScale = camHeight / (float)GetNearestPow2(camHeight);
    yTexScale = camWidth / (float)GetNearestPow2(camWidth);
  } else {
    xTexScale = camWidth / (float)GetNearestPow2(camWidth);
    yTexScale = camHeight / (float)GetNearestPow2(camHeight);
  }

  // Dispose of the previous drawImageBuffer
  vsReleaseImage(&drawImageBuffer);
  drawImageBuffer = 0;

  // Depending on the camera orientation (landscape or portrait), create a
  // drawImageBuffer buffer for storing pixels that will be used in the tracking
  // thread
  if (camOrientation == 90 || camOrientation == 270)
    drawImageBuffer = vsCreateImage(vsSize(height, width), VS_DEPTH_8U, 3);
  else
    drawImageBuffer = vsCreateImage(vsSize(width, height), VS_DEPTH_8U, 3);

  // Dispose of the previous drawImage
  vsReleaseImage(&renderImage);
  renderImage = 0;

  // Create a renderImage buffer based on the drawImageBuffer which will be used
  // in the rendering thread NOTE: Copying imageData between track and draw
  // buffers is protected with mutexes
  renderImage = vsCloneImage(drawImageBuffer);

  parametersChanged = true;
  pthread_mutex_unlock(&writeFrame_mutex);
  pthread_mutex_unlock(&grabFrame_mutex);

  LOGI("Java_app_specta_inc_camera_CameraActivity_setParameters");
}

void _getCameraInfo(float *focus, int *ImageWidth, int *ImageHeight) {
  *focus = trackingData[0].cameraFocus;
  if (camOrientation == 90 || camOrientation == 270) {
    *ImageWidth = camHeight;
    *ImageHeight = camWidth;
  } else {
    *ImageWidth = camWidth;
    *ImageHeight = camHeight;
  }
}

void _toggleTorch(int on) { LOGI("@@ QR _toggleTorch"); }

void _grabFrameInternal() {

  while (previewStarted) {
    if (imageCapture == 0) {
      // return;
      usleep(1000 * 100);
      continue;
    }
    pthread_mutex_lock(&grabFrame_mutex);
    long ts;
    trackImage = imageCapture->GrabFrame(ts);

    if (trackImage == 0) {
      // vsResize(renderImage, trackImage);
      pthread_mutex_unlock(&grabFrame_mutex);
      continue;
    }
    pthread_mutex_unlock(&grabFrame_mutex);

    // update the draw buffer image
    pthread_mutex_lock(&displayFrame_mutex);

    // copy current image to back buffer
    vsCopy(trackImage, drawImageBuffer);

    pthread_mutex_unlock(&displayFrame_mutex);
  }
}

void _grabFrame() {
  if (!previewStarted) {
    previewStarted = true;
    std::thread bgGrab(_grabFrameInternal);
    bgGrab.detach();
  }
}
void updateAnalyserEstimations() {

  pthread_mutex_lock(&grabFrame_mutex);
  detectedAge = m_FaceAnalizer->estimateAge(trackImage, trackingData);
  detectedGender = m_FaceAnalizer->estimateGender(trackImage, trackingData);
  pthread_mutex_unlock(&grabFrame_mutex);
  LOGI("#### updateAnalyserEstimations Detected age:%d and gender: %d",
       detectedAge, detectedGender);
}

int _track() {
  if (!scannerEnabled) {
    pthread_mutex_lock(&grabFrame_mutex);

    if (trackImage != 0) {
      // LOGI("#### Camera frame ready v2!");

      if (camOrientation == 90 || camOrientation == 270)
        trackingStatus = m_Tracker->track(
            trackHeight, trackWidth, (const char *)trackImage->imageData,
            trackingData, VISAGE_FRAMEGRABBER_FMT_RGB,
            VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);
      else
        trackingStatus = m_Tracker->track(
            trackWidth, trackHeight, (const char *)trackImage->imageData,
            trackingData, VISAGE_FRAMEGRABBER_FMT_RGB,
            VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);

      // g_TextureId
      if (trackingStatus[0] == TRACK_STAT_OFF) {
        trackImage = 0;
        // LOGI("#### TRACK_STAT_OFF  !");
        g_TextureId = -1;
      }
      if (ageRefreshRequested && m_FaceAnalizer &&
          trackingStatus[0] == TRACK_STAT_OK) {
        ageRefreshRequested = 0;

        // std::thread bgCheck(updateAnalyserEstimations);
        // bgCheck.detach();
      }
    } else {
      int res[] = {TRACK_STAT_OFF};

      trackingStatus = &res[0];
      LOGI("#### Camera frame not yet ready v2 :%d!", trackingStatus[0]);
    }

    // LOGI("# Native _track:%d", trackingStatus[0]);
    pthread_mutex_unlock(&grabFrame_mutex);
  } else {
    int res[] = {TRACK_STAT_RECOVERING};
    trackingStatus = &res[0];
  }
  return trackingStatus[0];
}

const char *_get3DData(float *tx, float *ty, float *tz, float *rx, float *ry,
                       float *rz) {
  if (trackingStatus[0] == TRACK_STAT_OK) {
    *tx = -trackingData[0].faceTranslation[0];
    *ty = trackingData[0].faceTranslation[1];
    *tz = trackingData[0].faceTranslation[2];
  } else {
    *tx = -10000.0f;
    *ty = 0.0f;
    *tz = 0.0f;
  }
  *rx = trackingData[0].faceRotation[0] * 180 / 3.14f;
  *ry = -(trackingData[0].faceRotation[1] + 3.14f) * 180.0f / 3.14f;
  *rz = -trackingData[0].faceRotation[2] * 180 / 3.14f;

  const char *tstatus;
  switch (trackingStatus[0]) {
  case TRACK_STAT_OFF:
    tstatus = "OFF";
    break;
  case TRACK_STAT_OK:
    tstatus = "OK";
    break;
  case TRACK_STAT_RECOVERING:
    tstatus = "RECOVERING";
    break;
  case TRACK_STAT_INIT:
    tstatus = "INITIALIZING";
    break;
  default:
    tstatus = "N/A";
    break;
  }
  char message[256];

  return MakeStringCopy(message);
}

void _bindTexture(int texID) {
  // binds a texture with the given native hardware texture id through opengl
  if (texID != -1 && renderImage != 0) {
    g_TextureId = texID;
    // just update the texture pointer - the drawing is now done on the render
    // event callback
  }
}
void drawPreviewFrame(const char *yuvBuffer);
void renderPreviewTexture();
void preparePreview(const char *yuvBuffer);

static void UNITY_INTERFACE_API OnRenderEvent(int eventID) {

  // Unknown / unsupported graphics device type? Do nothing
  if (s_Graphics == NULL) {
    return;
  }

  //***
  //*** LOCK track thread to copy data for rendering ***
  //***
  pthread_mutex_lock(&displayFrame_mutex);
  // copy image for rendering
  vsCopy(drawImageBuffer, renderImage);

  //***
  //*** UNLOCK track thread ***
  //***
  pthread_mutex_unlock(&displayFrame_mutex);

  // binds a texture with the given native hardware texture id through opengl
  if (g_TextureId != -1 && renderImage != 0) {

    preparePreview((const char *)renderImage->imageData);
    renderPreviewTexture();
    drawPreviewFrame((const char *)renderImage->imageData);
  }
}

void preparePreview(const char *yuvBuffer) {

  const char *uvBuffer = uvBuffer + camWidth * camHeight;
  // Setup the pixel alignement
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  printGLError("preparePreview");

  // Active the y texture
  glActiveTexture(GL_TEXTURE1);

  printGLError("preparePreview");
  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, GL_TEXTURE1);
  printGLError("preparePreview");

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, camWidth, camHeight, 0,
               GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvBuffer);
  printGLError("preparePreview:glTexImage2D");

  // Setup the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  printGLError("preparePreview");

  // Prepare the UV channel texture
  glActiveTexture(GL_TEXTURE2);
  printGLError("preparePreview");

  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, GL_TEXTURE2);
  printGLError("preparePreview");

  // Setup the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  printGLError("preparePreview");

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, camWidth, camHeight, 0,
               GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uvBuffer);

  glViewport(0, 0, camWidth, camHeight);
}

void renderPreviewTexture() {
  // Generate framebuffer object name
  glGenFramebuffers(1, &m_FrameBuffer);
  printGLError("glGenFramebuffers");

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
  printGLError("renderPreviewTexture");

  // Generate render buffer object name
  glGenRenderbuffers(1, &m_RenderBuffer);
  printGLError("renderPreviewTexture");

  // Bind render buffer
  glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBuffer);
  printGLError("renderPreviewTexture");

  // Create and initialize render buffer for display RGBAwith the same size of
  // the viewport
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, camWidth, camHeight);
  printGLError("renderPreviewTexture");

  // Attach render buffer to frame buffer object
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, m_RenderBuffer);
  printGLError("renderPreviewTexture");

  // Attach y plane to frame buffer object
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         g_TextureId, 0);
  printGLError("renderPreviewTexture");

  // Attach uv plane to frame buffer object
  //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
  //                       GL_TEXTURE2, 0);
  printGLError("renderPreviewTexture");

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  printGLError("renderPreviewTexture");

  // Check if the framebuffer is correctly setup
  GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    LOGI(" FBO setting fault : %d ", status);
    return;
  }
  printGLError("renderPreviewTexture");
}

void drawPreviewFrame(const char *yuvBuffer) {
  const char *uvBuffer = uvBuffer + camWidth * camHeight;
  LOGI("drawPreviewFrame");
  glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
  printGLError("glBindFramebuffer");
  glUseProgram(m_Program);
  printGLError("glUseProgram");

  // y plane buffer setup
  glActiveTexture(GL_TEXTURE1);
  printGLError("glActiveTexture");

  glBindTexture(GL_TEXTURE_2D, GL_TEXTURE1);
  printGLError("glBindTexture");

  glUniform1i(yTextureObject, 0);
  printGLError("glUniform1i");

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_LUMINANCE,
                  GL_UNSIGNED_BYTE, yuvBuffer);
  printGLError("glTexSubImage2D");

  // uv plane buffer setup
  glActiveTexture(GL_TEXTURE2);
  printGLError("glActiveTexture");

  glBindTexture(GL_TEXTURE_2D, GL_TEXTURE2);
  printGLError("glBindTexture");

  glUniform1i(uvTextureObject, 1);
  printGLError("glUniform1i");

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight,
                  GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uvBuffer);
  printGLError("glTexSubImage2D");


  glViewport(0, 0, camWidth, camHeight);
  printGLError("glTexSubImage2D");
  /*
  int triangleCount = 1;
  struct MyVertex {
    float x, y, z;
    unsigned int color;
  };
  MyVertex verticesFloat3Byte4[3] = {
      {-0.5f, -0.25f, 0, 0xFFff0000},
      {0.5f, -0.25f, 0, 0xFF00ff00},
      {0, 0.5f, 0, 0xFF0000ff},
  };

  // Bind a vertex buffer, and update data in it
  const int kVertexSize = 12 + 4;
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  printGLError("vertex buffer");
  glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
  printGLError("vertex buffer");
  glBufferSubData(GL_ARRAY_BUFFER, 0, kVertexSize * triangleCount * 3,
                  verticesFloat3Byte4);
  printGLError("vertex buffer");

  // Setup vertex layout
  glEnableVertexAttribArray(kVertexInputPosition);
  printGLError("vertex buffer");

  glVertexAttribPointer(positionObject, 3, GL_FLOAT, GL_FALSE, kVertexSize,
                        (char *)NULL + 0);
  printGLError("vertex buffer");

  glEnableVertexAttribArray(positionObject);
  printGLError("vertex buffer");

  glVertexAttribPointer(kVertexInputColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                        kVertexSize, (char *)NULL + 12);
  printGLError("vertex buffer");

  // Draw
  glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);

  printGLError("vertex buffer");


    glVertexAttribPointer(positionObject, 2, GL_FLOAT, GL_FALSE, kStride,
                          kVertexInformation);
    printGLError("glVertexAttribPointer");

    glVertexAttribPointer(texturePosition, 2, GL_SHORT, GL_FALSE, kStride,
                          kTextureCoordinateInformation);glDrawArrays
    printGLError("glVertexAttribPointer");

    glEnableVertexAttribArray(positionObject);
    printGLError("glVertexAttribArray");

    glEnableVertexAttribArray(texturePosition);
    printGLError("glVertexAttribArray");


          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          printGLError("glBindFramebuffer");

          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
          printGLError("glDrawElements");

          // eglSwapBuffers(display, surface);
          printGLError("eglSwapBuffers");  */
}
// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a
// rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc() {
  return OnRenderEvent;
}

void initializeOpenGL() {
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);

  GLuint renderedTexture;
  glGenTextures(1, &renderedTexture);
  textureID = renderedTexture;
  LOGI("# Native initializeOpenGL :%d", textureID);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID);

  glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int getTexturePointer() { return textureID; }

void resizeViewport(int newWidth, int newHeight) {
  glViewport(0, 0, newWidth, newHeight);
}
void renderFrame() { glClear(GL_COLOR_BUFFER_BIT); }

// a helper function to get all the needed info in one native call
bool _getFaceModel(int *vertexNumber, float *vertices, int *triangleNumber,
                   int *triangles, float *texCoord) {
  if (trackingStatus[0] == TRACK_STAT_OFF)
    return false;

  // get vertex number
  *vertexNumber = trackingData[0].faceModelVertexCount;

  // get vertices
  memcpy(vertices, trackingData[0].faceModelVertices,
         3 * (*vertexNumber) * sizeof(float));

  // get triangle number
  *triangleNumber = trackingData[0].faceModelTriangleCount;

  // get triangles in reverse order
  for (int i = 0; i < *triangleNumber * 3; i++) {
    triangles[*triangleNumber * 3 - 1 - i] =
        trackingData[0].faceModelTriangles[i];
  }

  // texture coordinates are normalized to frame
  // and because frame is only in the part of our texture
  // we must scale texture coordinates to match
  // frame_width/tex_width and frame_height/tex_height
  // also x-coord must be flipped
  for (int i = 0; i < *vertexNumber * 2; i += 2) {
    texCoord[i + 0] =
        (1.0f - trackingData[0].faceModelTextureCoords[i + 0]) * xTexScale;
    texCoord[i + 1] = trackingData[0].faceModelTextureCoords[i + 1] * yTexScale;
  }

  return true;
}

int _getActionUnitCount() {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return 0;

  return trackingData[0].actionUnitCount;
}

void _getActionUnitValues(float *values) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return;

  // get eye au indices
  int leftIndex = -1;
  int rightIndex = -1;
  for (int i = 0; i < trackingData[0].actionUnitCount; i++) {
    if (!strcmp(trackingData[0].actionUnitsNames[i], "au_leye_closed")) {
      leftIndex = i;
    }

    if (!strcmp(trackingData[0].actionUnitsNames[i], "au_reye_closed")) {
      rightIndex = i;
    }

    if (leftIndex >= 0 && rightIndex >= 0)
      break;
  }

  // if action units for eye closure are not used by the tracker, map eye
  // closure values to them
  if (leftIndex >= 0 && trackingData[0].actionUnitsUsed[leftIndex] == 0) {
    trackingData[0].actionUnits[leftIndex] = trackingData[0].eyeClosure[0];
  }
  if (rightIndex >= 0 && trackingData[0].actionUnitsUsed[rightIndex] == 0) {
    trackingData[0].actionUnits[rightIndex] = trackingData[0].eyeClosure[1];
  }

  memcpy(values, trackingData[0].actionUnits,
         trackingData[0].actionUnitCount * sizeof(float));
}

const char *_getActionUnitName(int index) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return MakeStringCopy("");

  return trackingData[0].actionUnitsNames[index];
}

bool _getActionUnitUsed(int index) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return false;

  return trackingData[0].actionUnitsUsed[index] == 1;
}

bool _getGazeDirection(float *direction) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return false;

  memcpy(direction, trackingData[0].gazeDirection, 2 * sizeof(float));
  return true;
}

bool _getFeaturePoints2D(int number, int *groups, int *indices,
                         float *positions) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return false;

  for (int i = 0; i < number; i++) {
    int group = groups[i];
    int index = indices[i];
    const float *position =
        trackingData[0].featurePoints2D->getFPPos(group, index);
    positions[i * 2] = position[0];
    positions[i * 2 + 1] = position[1];
  }

  return true;
}

bool _getFeaturePoints3D(int number, int *groups, int *indices,
                         float *positions) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return false;

  for (int i = 0; i < number; i++) {
    int group = groups[i];
    int index = indices[i];
    const float *position =
        trackingData[0].featurePoints3D->getFPPos(group, index);
    positions[i * 3] = position[0];
    positions[i * 3 + 1] = position[1];
    positions[i * 3 + 2] = position[2];
  }

  return true;
}

bool _getFeaturePoints3DRel(int number, int *groups, int *indices,
                            float *positions) {
  if (trackingStatus[0] != TRACK_STAT_OK)
    return false;

  for (int i = 0; i < number; i++) {
    int group = groups[i];
    int index = indices[i];
    const float *position =
        trackingData[0].featurePoints3DRelative->getFPPos(group, index);
    positions[i * 3] = position[0];
    positions[i * 3 + 1] = position[1];
    positions[i * 3 + 2] = position[2];
  }

  return true;
}

void Java_app_specta_inc_camera_CameraActivity_onCodeDetected(
    JNIEnv *env, jobject obj, jstring code, jfloat top, jfloat left,
    jfloat bottom, jfloat right) {

  const char *qrCode = env->GetStringUTFChars(code, NULL);
  LOGI("##### Java_app_specta_inc_camera_CameraActivity_onCodeDetected : %s",
       qrCode);

  if (scanCallback != NULL) {
    scanCallback(qrCode, (float)top, (float)left, (float)bottom, (float)right);
  }
}

void _initScanner(transitionCallback initCallback, callbackFunc callback) {

  LOGI("@@ QR _initScanner v4.0");

  initScannerCallback = initCallback;

  scannerEnabled = true;
  scanCallback = callback;
  switchingCamera = true;

  jni_env = 0;
  _vm->AttachCurrentThread(&jni_env, 0);
  jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
  jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
                                                    "Landroid/app/Activity;");

  obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

  LOGI("@@ QR _initScanner - activity ready to be used");

  jclass dataClass = jni_env->GetObjectClass(obj_Activity);
  if (jni_env->ExceptionCheck())
    jni_env->ExceptionClear();
  if (dataClass != NULL) {
    jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
    jmethodID javaMethodRef =
        jni_env->GetMethodID(javaClassRef, "startScanner", "(I)I");

    if (jni_env->ExceptionCheck())
      jni_env->ExceptionClear();
    if (javaMethodRef != 0) {
      LOGI("@@ QR _initScanner - BEFORE JAVA CALL");
      jni_env->CallIntMethod(obj_Activity, javaMethodRef, (jint)1);
    } else {

      LOGI("@@ QR _initScanner - startScanner - Method not found");
    }

    jni_env->DeleteGlobalRef(javaClassRef);
  } else {

    LOGI("@@ QR _initScanner - dataClass is NULL");
  }

  LOGI("@@ QR _initScanner - Returning to unity");
}

/** Releases memory allocated by the scanner in the initScanner function.
 */
void _releaseScanner(transitionCallback callback) {

  LOGI("@@ QR _releaseScanner v2");
  switchingCamera = true;

  LOGI("@@ QR _releaseScanner - in mutex");
  releaseScannerCallback = callback;

  jni_env = 0;
  _vm->AttachCurrentThread(&jni_env, 0);
  jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
  jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
                                                    "Landroid/app/Activity;");

  obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

  LOGI("@@ QR _releaseScanner - activity ready to be used");

  LOGI("@@ QR _releaseScanner NDK");
  jclass dataClass = jni_env->GetObjectClass(obj_Activity);
  if (jni_env->ExceptionCheck())
    jni_env->ExceptionClear();
  if (dataClass != NULL) {
    jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
    jmethodID javaMethodRef =
        jni_env->GetMethodID(javaClassRef, "releaseScanner", "(I)I");

    if (jni_env->ExceptionCheck())
      jni_env->ExceptionClear();
    if (javaMethodRef != 0) {
      jni_env->CallIntMethod(obj_Activity, javaMethodRef, (jint)2);
    }

    jni_env->DeleteGlobalRef(javaClassRef);
  }

  scannerEnabled = false;
}
}