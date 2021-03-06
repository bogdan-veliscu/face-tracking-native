//
//  VisageTrackerUnityPlugin.cpp
//  VisageTrackerUnityPlugin
//
//
#include "Unity/IUnityGraphics.h"
#include "VisageTrackerUnityPlugin.h"
#include "VisageTracker.h"
#include "VisageFaceAnalyser.h"
#include "Common.h"
#include "WrapperOpenCV.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "AndroidCameraCapture.h"
#include <jni.h>
#include <unistd.h>
#include <thread>
#include <future>
#include <android/log.h>
#define  LOG_TAG    "UnityPlugin v3"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)



using namespace VisageSDK;

char* trackerConfig;
char* trackerLicense;
char* analyserConfig;
char* analyserLicense;

static void* g_TextureHandle = NULL;

static int   g_TextureWidth = 0;
static int   g_TextureHeight = 0;

static GLuint frame_tex_id = 0;

// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity(float t) { g_Time = t; }


// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of the scripts.


// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{

	LOGI("### VisageFaceAnalyser - UnityPluginLoad");
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);


	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	//LOGI("### VisageFaceAnalyser - UnityPluginUnload");
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTextureFromUnity(void* textureHandle, int w, int h)
{
	// A script calls this at initialization time; just remember the texture pointer here.
	// Will update texture pixels each frame from the plugin rendering event (texture update
	// needs to happen on the rendering thread).
	LOGI("### VisageFaceAnalyser - SetTextureFromUnity");
	g_TextureHandle = textureHandle;
	g_TextureWidth = w;
	g_TextureHeight = h;

	GLuint frame_tex_id = (GLuint)(size_t)(g_TextureHandle);
}
// --------------------------------------------------------------------------
// GraphicsDeviceEvent

static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;


static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	//LOGI("### VisageFaceAnalyser - OnGraphicsDeviceEvent");
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		s_DeviceType = s_Graphics->GetRenderer();
		LOGI("### VisageFaceAnalyser - kUnityGfxDeviceEventInitialize");
	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		s_DeviceType = kUnityGfxRendererNull;
	}
}


static int g_TextureId = -1;

static callbackFunc scanCallback;

static transitionCallback trackerInitCallback;
static transitionCallback analyserInitCallback;

static VisageTracker* m_Tracker = 0;
static VisageFaceAnalyser* m_FaceAnalizer = 0;

static int detectedAge = -2;
static int detectedGender = -2;
static int ageRefreshRequested = 1;

int format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
static VsImage *pixelData = 0;
static VsImage *renderImage = 0;

static AndroidCameraCapture *imageCapture = 0;

// Helper method to create C string copy
static char* MakeStringCopy(const char* val)
{
	if (val == NULL)
		return NULL;

	char* res = (char*)malloc(strlen(val) + 1);
	strcpy(res, val);
	return res;
}

static unsigned int GetNearestPow2(unsigned int num)
{
	unsigned int n = num > 0 ? num - 1 : 0;

	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;

	return n;
}


namespace VisageSDK
{
	void initializeLicenseManager(JNIEnv* env, jobject obj, const char *licenseKeyFileName,
		void(*alertFunction)(const char*) = 0);
}


// When native code plugin is implemented in .mm / .cpp file, then functions
// should be surrounded with extern "C" block to conform C function naming rules
extern "C" {
	
	static FaceData trackingData[10];
	static int *trackingStatus = TRACK_STAT_OFF;
	//
	static int camWidth = 0;
	static int camHeight = 0;
	static int camOrientation = 0;
	static int camFlip = 0;
	static float xTexScale;
	static float yTexScale;
	//
	static bool parametersChanged;

	static bool scannerEnabled;
	static bool scannerWaitingForFrames;

	static bool switchingCamera;
    static float * detectedEmotions = new float [7];

	transitionCallback initScannerCallback;
	transitionCallback releaseScannerCallback;

	pthread_mutex_t writeFrame_mutex;
	pthread_mutex_t grabFrame_mutex;
	pthread_mutex_t displayFrame_mutex;

	JavaVM* _vm = 0;
	JNIEnv* jni_env = 0;
	jobject obj_Activity;

	static int textureID = 0;

	void AlertCallback(const char* warningMessage)
	{
		jclass dataClass = jni_env->GetObjectClass(obj_Activity);
		if (jni_env->ExceptionCheck())
			jni_env->ExceptionClear();
		if (dataClass != NULL)
		{
			jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
			jmethodID javaMethodRef = jni_env->GetMethodID(javaClassRef, "AlertDialogFunction",
				"(Ljava/lang/String;)V");
			if (jni_env->ExceptionCheck())
				jni_env->ExceptionClear();
			jstring message = jni_env->NewStringUTF(warningMessage);
			if (javaMethodRef != 0)
			{
				jni_env->CallVoidMethod(obj_Activity, javaMethodRef, message);
			}

			jni_env->DeleteGlobalRef(javaClassRef);
			jni_env->DeleteLocalRef(message);
		}
	}

	//Example how to obtain JNI environment and activity of this Unity sample required for license check.
	jint JNI_OnLoad(JavaVM* vm, void* reserved)
	{
		_vm = vm;

		return JNI_VERSION_1_6;
	}


	void _initTracker(char* configuration, char* license)
	{
		_initTrackerWithCallback(configuration, license, nullptr);
		LOGI("### _initTracker v3");
	}

	void asyncInitTrackerWithCallback() {
		jni_env = 0;
		_vm->AttachCurrentThread(&jni_env, 0);
		jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
		jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
			"Landroid/app/Activity;");
		obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

		LOGI("_initTracker with license: %s", trackerLicense);
		initializeLicenseManager(jni_env, obj_Activity, trackerLicense, AlertCallback);

		if (m_Tracker)
			_releaseTracker();

		m_Tracker = new VisageTracker(trackerConfig);
		pthread_mutex_init(&writeFrame_mutex, NULL);
		pthread_mutex_init(&grabFrame_mutex, NULL);

		//Set up mutex for render thread synchronization
		pthread_mutex_destroy(&displayFrame_mutex);
		pthread_mutex_init(&displayFrame_mutex, NULL);

		if (trackerInitCallback != NULL) {
			LOGI("### _initTracker callback");
			trackerInitCallback();
		}
	}

	void _initTrackerWithCallback(char* config, char* license, transitionCallback callback) {
		trackerConfig = MakeStringCopy(config);
		trackerLicense = MakeStringCopy(license);
		trackerInitCallback = callback;

		auto asyncCall = std::async(asyncInitTrackerWithCallback);
		asyncCall.get();
	}

	void _releaseTracker()
	{
		delete m_Tracker;
		delete m_FaceAnalizer;

		/*
				pthread_mutex_lock(&displayFrame_mutex);
				  if(pixelDataRaw != 0){
					delete[](unsigned char*)pixelDataRaw;
					pixelDataRaw = 0;
				  }
			   pthread_mutex_unlock(&displayFrame_mutex);
		*/

		m_Tracker = 0;
	}

	void _initFaceAnalyser(char* config, char* license) {
		_initFaceAnalyserWithCallback(config, license, nullptr);
	}

	void asyncInitFaceAnalyserWithCallback() {
		jni_env = 0;
		_vm->AttachCurrentThread(&jni_env, 0);
		jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
		jfieldID fid_Activity = jni_env->GetStaticFieldID(unity, "currentActivity",
			"Landroid/app/Activity;");
		obj_Activity = jni_env->GetStaticObjectField(unity, fid_Activity);

		initializeLicenseManager(jni_env, obj_Activity, analyserLicense, AlertCallback);

		if (m_FaceAnalizer) {
			delete m_FaceAnalizer;
		}
		m_FaceAnalizer = new VisageFaceAnalyser();
		LOGI("@@ VisageFaceAnalyser init with config: %s\n", analyserConfig);
		int ret = m_FaceAnalizer->init(analyserConfig);
		LOGI("### VisageFaceAnalyser _initFaceAnalyser :%d", ret);

		if (analyserInitCallback != NULL) {


			LOGI("### VisageFaceAnalyser _initFaceAnalyser CALLBACK : %d", ret);
			analyserInitCallback();
		}
	}

	void _initFaceAnalyserWithCallback(char* config, char* license, transitionCallback callbackFunc) {

		analyserConfig = MakeStringCopy(config);
		analyserLicense = MakeStringCopy(license);
		analyserInitCallback = callbackFunc;

		auto asyncCall = std::async(asyncInitFaceAnalyserWithCallback);
		asyncCall.get();
	}

	void _refreshAgeEstimate() {
		ageRefreshRequested = 1;
	}

	int _estimateAge() {
		return detectedAge;
	}
	int _estimateGender() {
		return detectedGender;
	}

    void _estimateEmotion(float *emotions){
        for (int i=0; i<7; i++) {
            emotions[i] = detectedEmotions[i];
        }
    }

	void Java_app_specta_inc_camera_CameraActivity_WriteFrame(JNIEnv *env, jobject obj, jbyteArray frame)
	{

		pthread_mutex_lock(&writeFrame_mutex);
		if (!parametersChanged) {
			jbyte *pixelData = env->GetByteArrayElements(frame, 0);

			// LOGI("Java_app_specta_inc_camera_CameraActivity_WriteFrame : %d", pixelData);

			imageCapture->WriteFrameYUV((unsigned char*)pixelData);
			env->ReleaseByteArrayElements(frame, pixelData, 0);
		}
		else {
			LOGI("Write frame after changing parameters");
			switchingCamera = false;
		}

		parametersChanged = false;
		//LOGI("Java_app_specta_inc_camera_CameraActivity_WriteFrame - END");
		pthread_mutex_unlock(&writeFrame_mutex);

		if (scannerWaitingForFrames && scannerEnabled == false) {
			scannerEnabled = true;
			LOGI("Java_app_specta_inc_camera_CameraActivity_init - START");

		}
	}

	void Java_app_specta_inc_camera_CameraActivity_setScannerEnabled(JNIEnv *env, jobject obj, jint enabled) {
		LOGI("### Scanner callback time: - just returned from JAVA2");

		scannerWaitingForFrames = enabled != 0;

		scannerEnabled = false;
		if (scannerWaitingForFrames) {
			if (initScannerCallback != NULL) {
				initScannerCallback();
			}
		}
		else {
			if (releaseScannerCallback != NULL) {
				releaseScannerCallback();
			}
		}

	}
	void Java_app_specta_inc_camera_CameraActivity_setParameters(JNIEnv *env, jobject obj, jint orientation,
		jint width, jint height, jint flip)
	{
		pthread_mutex_lock(&grabFrame_mutex);
		pthread_mutex_lock(&writeFrame_mutex);
		if (width != -1)
			camWidth = width;
		if (height != -1)
			camHeight = height;
		if (orientation != -1) {
			camOrientation = orientation;
			//LOGI("#### setParameters: %d ", orientation);
		}
		if (flip != -1)
			camFlip = flip;

		delete imageCapture;
		imageCapture = new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);


		if (camOrientation == 90 || camOrientation == 270)
		{
			xTexScale = camHeight / (float)GetNearestPow2(camHeight);
			yTexScale = camWidth / (float)GetNearestPow2(camWidth);
		}
		else
		{
			xTexScale = camWidth / (float)GetNearestPow2(camWidth);
			yTexScale = camHeight / (float)GetNearestPow2(camHeight);
		}

		parametersChanged = true;
		pthread_mutex_unlock(&writeFrame_mutex);
		pthread_mutex_unlock(&grabFrame_mutex);

		LOGI("Java_app_specta_inc_camera_CameraActivity_setParameters");
	}

	void _getCameraInfo(float *focus, int *ImageWidth, int *ImageHeight)
	{
		*focus = trackingData[0].cameraFocus;
		if (camOrientation == 90 || camOrientation == 270)
		{
			*ImageWidth = camHeight;
			*ImageHeight = camWidth;
		}
		else
		{
			*ImageWidth = camWidth;
			*ImageHeight = camHeight;
		}
	}

	void _grabFrameInternal()
	{
		if (imageCapture == 0)
			return;
		pthread_mutex_lock(&grabFrame_mutex);
		long ts;
		pixelData = imageCapture->GrabFrame(ts);
		/*

		 LOGI("#### _grabFrameInternal ---> 1");
		int frameSize = camHeight * camWidth * 3;
		pthread_mutex_lock(&displayFrame_mutex);
		if(switchingCamera){
		   memset(pixelDataRaw, 0, frameSize);
		   imageCapture->framesToFade = imageCapture->maxFramesToFade;
			LOGI("#### _grabFrameInternal ---> 2");
		} else if (pixelData != 0 && pixelDataRaw != 0){
		 LOGI("#### _grabFrameInternal ---> 2");
		  memcpy(pixelDataRaw, pixelData->imageData, (frameSize)*sizeof(char));
		   LOGI("#### _grabFrameInternal ---> 3");
		}

		pthread_mutex_unlock(&displayFrame_mutex);
  */
		if (pixelData == 0) {
			LOGI("#### _grabFrameInternal is 0");

		}

		pthread_mutex_unlock(&grabFrame_mutex);
	}

	void _grabFrame() {
		std::thread bgGrab(_grabFrameInternal);
		bgGrab.detach();
	}
	void updateAnalyserEstimations() {

		pthread_mutex_lock(&grabFrame_mutex);
		detectedAge = m_FaceAnalizer->estimateAge(pixelData, trackingData);
		detectedGender = m_FaceAnalizer->estimateGender(pixelData, trackingData);
		m_FaceAnalizer->estimateEmotion(pixelData, trackingData, detectedEmotions);

		pthread_mutex_unlock(&grabFrame_mutex);
		LOGI("#### updateAnalyserEstimations Detected age:%d and gender: %d", detectedAge, detectedGender);
	}

	int _track()
	{
		if (!scannerEnabled) {
			pthread_mutex_lock(&grabFrame_mutex);

			if (pixelData != 0) {
				//LOGI("#### Camera frame ready v2!");

				if (camOrientation == 90 || camOrientation == 270)
					trackingStatus = m_Tracker->track(camHeight, camWidth, (const char*)pixelData->imageData, trackingData,
						VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);
				else
					trackingStatus = m_Tracker->track(camWidth, camHeight, (const char*)pixelData->imageData, trackingData,
						VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);

				//g_TextureId
				if (trackingStatus[0] == TRACK_STAT_OFF) {
					pixelData = 0;
					//LOGI("#### TRACK_STAT_OFF  !");
					g_TextureId = -1;
				}
				if (ageRefreshRequested && m_FaceAnalizer && trackingStatus[0] == TRACK_STAT_OK) {
					ageRefreshRequested = 0;
					int frameSize = camHeight * camWidth * 3;

					//std::thread bgCheck(updateAnalyserEstimations);
				   // bgCheck.detach();
				}
			}
			else {
				int res[] = { TRACK_STAT_OFF };

				trackingStatus = &res[0];
				LOGI("#### Camera frame not yet ready v2 :%d!", trackingStatus[0]);
			}

			//LOGI("# Native _track:%d", trackingStatus[0]);
			pthread_mutex_unlock(&grabFrame_mutex);
		}
		else {
			int res[] = { TRACK_STAT_RECOVERING };
			trackingStatus = &res[0];
		}
		return trackingStatus[0];
	}


	const char* _get3DData(float* tx, float* ty, float* tz, float* rx, float* ry, float* rz)
	{
		if (trackingStatus[0] == TRACK_STAT_OK) {
			*tx = -trackingData[0].faceTranslation[0];
			*ty = trackingData[0].faceTranslation[1];
			*tz = trackingData[0].faceTranslation[2];
		}
		else {
			*tx = -10000.0f;
			*ty = 0.0f;
			*tz = 0.0f;
		}
		*rx = trackingData[0].faceRotation[0] * 180 / 3.14f;
		*ry = -(trackingData[0].faceRotation[1] + 3.14f)*180.0f / 3.14f;
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

	void _bindTexture(int texID)
	{
		// binds a texture with the given native hardware texture id through opengl
		if (texID != -1 && pixelData != 0)
		{
			//LOGI("### Native _bindTexture :%d", texID);
			g_TextureId = texID;
			//			glBindTexture(GL_TEXTURE_2D, texID);
			//			if (camOrientation == 90 || camOrientation == 270)
			//				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camHeight, camWidth, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
			//			else
			//				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
		}
	}

	static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
	{

		//LOGI("### Native OnRenderEvent :%d", eventID);
		// Unknown / unsupported graphics device type? Do nothing
		if (s_Graphics == NULL) {
			return;
			//LOGI("### Native OnRenderEvent [NOT READY] :%d", eventID);
		}
		pthread_mutex_lock(&grabFrame_mutex);
		// binds a texture with the given native hardware texture id through opengl
		if (g_TextureId != -1 && pixelData != 0)
		{
			glBindTexture(GL_TEXTURE_2D, g_TextureId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixelData->width, pixelData->height, GL_RGB, GL_UNSIGNED_BYTE, (const char*)pixelData->imageData);
			//LOGI("# Native OnRenderEvent - END ");
		}
		else {
			//LOGI("# Native OnRenderEvent - IGNORED : %d ", g_TextureId );
		}
		pthread_mutex_unlock(&grabFrame_mutex);
	}

	// --------------------------------------------------------------------------
	// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

	extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
	{
		return OnRenderEvent;
	}


	void initializeOpenGL()
	{
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

	int getTexturePointer() {
		return textureID;
	}

	void resizeViewport(int newWidth, int newHeight)
	{
		glViewport(0, 0, newWidth, newHeight);
	}
	void renderFrame()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// a helper function to get all the needed info in one native call
	bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord)
	{
		if (trackingStatus[0] == TRACK_STAT_OFF)
			return false;

		// get vertex number
		*vertexNumber = trackingData[0].faceModelVertexCount;

		// get vertices
		memcpy(vertices, trackingData[0].faceModelVertices, 3 * (*vertexNumber) * sizeof(float));

		// get triangle number
		*triangleNumber = trackingData[0].faceModelTriangleCount;

		// get triangles in reverse order
		for (int i = 0; i < *triangleNumber * 3; i++)
		{
			triangles[*triangleNumber * 3 - 1 - i] = trackingData[0].faceModelTriangles[i];
		}

		//texture coordinates are normalized to frame
		//and because frame is only in the part of our texture
		//we must scale texture coordinates to match
		//frame_width/tex_width and frame_height/tex_height
		//also x-coord must be flipped
		for (int i = 0; i < *vertexNumber * 2; i += 2) {
			texCoord[i + 0] = (1.0f - trackingData[0].faceModelTextureCoords[i + 0]) * xTexScale;
			texCoord[i + 1] = trackingData[0].faceModelTextureCoords[i + 1] * yTexScale;
		}

		return true;
	}

	int _getActionUnitCount()
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return 0;

		return trackingData[0].actionUnitCount;
	}

	void _getActionUnitValues(float* values)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return;

		// get eye au indices
		int leftIndex = -1;
		int rightIndex = -1;
		for (int i = 0; i < trackingData[0].actionUnitCount; i++)
		{
			if (!strcmp(trackingData[0].actionUnitsNames[i], "au_leye_closed"))
			{
				leftIndex = i;
			}

			if (!strcmp(trackingData[0].actionUnitsNames[i], "au_reye_closed"))
			{
				rightIndex = i;
			}

			if (leftIndex >= 0 && rightIndex >= 0)
				break;
		}

		// if action units for eye closure are not used by the tracker, map eye closure values to them
		if (leftIndex >= 0 && trackingData[0].actionUnitsUsed[leftIndex] == 0) {
			trackingData[0].actionUnits[leftIndex] = trackingData[0].eyeClosure[0];
		}
		if (rightIndex >= 0 && trackingData[0].actionUnitsUsed[rightIndex] == 0) {
			trackingData[0].actionUnits[rightIndex] = trackingData[0].eyeClosure[1];
		}

		memcpy(values, trackingData[0].actionUnits, trackingData[0].actionUnitCount * sizeof(float));
	}

	const char* _getActionUnitName(int index)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return MakeStringCopy("");

		return trackingData[0].actionUnitsNames[index];
	}

	bool _getActionUnitUsed(int index)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;

		return trackingData[0].actionUnitsUsed[index] == 1;
	}

	bool _getGazeDirection(float* direction)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;

		memcpy(direction, trackingData[0].gazeDirection, 2 * sizeof(float));
		return true;
	}

	bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData[0].featurePoints2D->getFPPos(group, index);
			positions[i * 2] = position[0];
			positions[i * 2 + 1] = position[1];
		}

		return true;
	}

	bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData[0].featurePoints3D->getFPPos(group, index);
			positions[i * 3] = position[0];
			positions[i * 3 + 1] = position[1];
			positions[i * 3 + 2] = position[2];
		}

		return true;
	}

	// returns the global feature point position, indication of whether the point is defined and detected and quality for all feature points (6 float values for every feature point)
// the method assumes the featurePointArray has been allocated by the callee
	bool _getAllFeaturePoints3D(ActionUnitStruct* featurePointArray, int length)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;
			int index = 0;
		for (int groupIndex = FDP::FP_START_GROUP_INDEX; groupIndex <= FDP::FP_END_GROUP_INDEX; ++groupIndex)
		{
			for (int pointIndex = 1; pointIndex <= FDP::groupSize(groupIndex); ++pointIndex)
			{
				if (index < length)
				{

					const float* positions3 = trackingData->featurePoints3D->getFPPos(groupIndex, pointIndex);
					featurePointArray[index].posX = positions3[0];
					featurePointArray[index].posY = positions3[1];
					featurePointArray[index].posZ = positions3[2];

					const FeaturePoint fp = trackingData->featurePoints3D->getFP(groupIndex, pointIndex);

					featurePointArray[index].defined = fp.defined;
					featurePointArray[index].detected = fp.detected;
					featurePointArray[index].quality = fp.quality;

					featurePointArray[index].index = pointIndex;
					featurePointArray[index].group = groupIndex;
					index++;
				}
				else {
					break;
				}
			}
		}

		return true;
	}

	bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions)
	{
		if (trackingStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData[0].featurePoints3DRelative->getFPPos(group, index);
			positions[i * 3] = position[0];
			positions[i * 3 + 1] = position[1];
			positions[i * 3 + 2] = position[2];
		}

		return true;
	}


	void Java_app_specta_inc_camera_CameraActivity_onCodeDetected(JNIEnv *env, jobject obj, jstring code, jfloat top, jfloat left, jfloat bottom, jfloat right) {

		const char *qrCode = env->GetStringUTFChars(code, NULL);
		LOGI("##### Java_app_specta_inc_camera_CameraActivity_onCodeDetected : %s", qrCode);

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
		if (dataClass != NULL)
		{
			jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
			jmethodID javaMethodRef = jni_env->GetMethodID(javaClassRef, "startScanner", "(I)I");

			if (jni_env->ExceptionCheck())
				jni_env->ExceptionClear();
			if (javaMethodRef != 0)
			{
				LOGI("@@ QR _initScanner - BEFORE JAVA CALL");
				jni_env->CallIntMethod(obj_Activity, javaMethodRef, (jint)1);
			}
			else {

				LOGI("@@ QR _initScanner - startScanner - Method not found");
			}

			jni_env->DeleteGlobalRef(javaClassRef);
		}
		else {

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
		if (dataClass != NULL)
		{
			jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
			jmethodID javaMethodRef = jni_env->GetMethodID(javaClassRef, "releaseScanner",
				"(I)I");

			if (jni_env->ExceptionCheck())
				jni_env->ExceptionClear();
			if (javaMethodRef != 0)
			{
				jni_env->CallIntMethod(obj_Activity, javaMethodRef, (jint)2);
			}

			jni_env->DeleteGlobalRef(javaClassRef);
		}

		scannerEnabled = false;

	}

	void _toggleTorch(int on) {
		LOGI("@@ QR _toggleTorch");
	}

	void _tapToFocus(float x, float y) {
	    string position = toString(x) + "x" + toString(y);
		jclass dataClass = jni_env->GetObjectClass(obj_Activity);
		if (jni_env->ExceptionCheck())
			jni_env->ExceptionClear();
		if (dataClass != NULL)
		{
			jclass javaClassRef = (jclass)jni_env->NewGlobalRef(dataClass);
			jmethodID javaMethodRef = jni_env->GetMethodID(javaClassRef, "focusOnPoint",
				"(Ljava/lang/String;)V");
			if (jni_env->ExceptionCheck())
				jni_env->ExceptionClear();
			jstring message = jni_env->NewStringUTF(position.c_str());
			if (javaMethodRef != 0)
			{
				jni_env->CallVoidMethod(obj_Activity, javaMethodRef, message);
			}

			jni_env->DeleteGlobalRef(javaClassRef);
			jni_env->DeleteLocalRef(message);
		}
	}
}