//
//  VisageTrackerUnityPlugin.cpp
//  VisageTrackerUnityPlugin
//
//
#include "VisageTrackerUnityPlugin.h"
#include "AndroidCameraCapture.h"
#include "Common.h"
#include "FrameRenderer.h"
#include "Unity/IUnityGraphics.h"
#include "VisageFaceAnalyser.h"
#include "VisageTracker.h"
#include "WrapperOpenCV.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <jni.h>
#include <thread>
#include <unistd.h>

#include "GLUtils.h"
#include <iostream>

#include <assert.h>

#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace VisageSDK;

static bool previewStarted = false;

static void *g_TextureHandle = NULL;

static int g_TextureWidth = 0;
static int g_TextureHeight = 0;

static GLuint frame_tex_id = 0;
static int g_TextureId = -1;

static FrameRenderer *renderer = 0;

unsigned char *rawFrameBuffer = 0;
pthread_mutex_t grabFrame_mutex;

// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the
// scripts.

static float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
SetTimeFromUnity(float t) {
  g_Time = t;
}



/**
 * Simple timer function
 */
long getTimestamp() {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (long) ((now.tv_sec*1000000000LL + now.tv_nsec)/1000000LL);
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

    LOGI("### Conversion shader initialized!");
  }

  // Cleanup graphics API implementation upon shutdown
  if (eventType == kUnityGfxDeviceEventShutdown) {
    s_DeviceType = kUnityGfxRendererNull;
  }
}

static callbackFunc scanCallback;

static VisageTracker *m_Tracker = 0;
static VisageFaceAnalyser *m_FaceAnalizer = 0;

static int detectedAge = -2;
static int detectedGender = -2;
static int ageRefreshRequested = 1;

int format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
static VsImage *trackImage = 0;

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

static bool cameraFrameArrived = false;

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
  pthread_mutex_init(&grabFrame_mutex, NULL);


}

void _releaseTracker() {
  delete m_Tracker;
  delete m_FaceAnalizer;

  LOGI("@@ QR _releaseTracker v2");
  pthread_mutex_lock(&grabFrame_mutex);
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

unsigned char* copyToCharArray(JNIEnv *env, jbyteArray array,unsigned char * buffer) {
    int len = env->GetArrayLength (array);
    if(buffer == 0){
      buffer = new unsigned char[len];
    }
    env->GetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buffer));
    //LOGI("Write frame - completed %d bytes", len);
    return buffer;
}

JNIEXPORT void Java_app_specta_inc_camera_CameraActivity_draw(JNIEnv * env, jobject obj, jbyteArray data, jint width, jint height, jint rotation)
{
  jbyte* bufferPtr = env->GetByteArrayElements(data, 0);

  jsize arrayLength = env->GetArrayLength(data);

  if (camWidth != width) {
    camWidth = width;
    camHeight = height;
    camOrientation = rotation;
    pthread_mutex_lock(&grabFrame_mutex);
    delete imageCapture;
    imageCapture =
        new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);
    pthread_mutex_unlock(&grabFrame_mutex);
  }

  //LOGE("@ Java_app_specta_inc_camera_CameraActivity_draw : %d x %d", width, height );
  if (renderer == 0) {
    LOGI("# Frame Renderer not yet initialized -- initializing now ..");
    renderer = new FrameRenderer();
  }

  if (renderer != nullptr) {
    renderer->draw((uint8_t *)bufferPtr, (size_t)arrayLength, (size_t)width,
                     (size_t)height, rotation);
  } else {
    LOGI("Java_com_android_gles3jni_GLES3JNILib_draw --> FrameRenderer is NULL");
  }

  env->ReleaseByteArrayElements(data, bufferPtr, 0);
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
    if (imageCapture == 0 || g_TextureId == -1) {
      // return;
      usleep(1000 * 100);
      continue;
    }
    pthread_mutex_lock(&grabFrame_mutex);
    long ts;
    trackImage = imageCapture->GrabFrame(ts);
    pthread_mutex_unlock(&grabFrame_mutex);
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
        //g_TextureId = -1;
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

void _bindTexture(int texID, int width, int height) {
  // binds a texture with the given native hardware texture id through opengl
  if (texID != -1 && g_TextureId != texID) {
    LOGI("# _bindTexture  .. %d to screen : %d x %d", texID, width, height);
    g_TextureId = texID;
    // just update the texture pointer - the drawing is now done on the render
    // event callback

    if (renderer == 0) {
      LOGI("# Frame Renderer not yet initialized -- initializing now ..");
      renderer = new FrameRenderer();
    }
    renderer->init(width, height, g_TextureId);
  }
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID) {

  // LOGI("# OnRenderEvent ..");
  // Unknown / unsupported graphics device type? Do nothing
  if (s_Graphics == NULL) {
    usleep(1000 * 111);
    return;
  }

  // binds a texture with the given native hardware texture id through opengl
  if (g_TextureId != -1) {

    if (renderer == 0) {
      LOGI("# Frame Renderer not yet initialized .. returning ");
      return;
    }

    cameraFrameArrived = false;

    //LOGI("# renderer->renderFrame(%p, %d) ", &rawFrameBuffer, g_TextureId);
    long start = getTimestamp();
    unsigned char *img = renderer->render();
    LOGI("### Frame rendering took : %d ", (int)(getTimestamp() - start));

    imageCapture->WriteFrame(img);
    //delete[] img;
    long end = getTimestamp();

    LOGI("### Frame conversion took : %d ", (int)(end - start));

  } else {
    LOGE("# Unity texture not yet ready");
    usleep(500);
  }
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a
// rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc() {
  return OnRenderEvent;
}
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