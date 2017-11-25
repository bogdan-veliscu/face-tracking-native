//
//  VisageTrackerUnityPlugin.cpp
//  VisageTrackerUnityPlugin
//
//

#include "VisageTrackerUnityPlugin.h"
#include "VisageTracker.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "AndroidCameraCapture.h"
#include <jni.h>
#include <unistd.h>

#include <android/log.h>
#define  LOG_TAG    "UnityPlugin"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)



using namespace VisageSDK;

static VisageTracker* m_Tracker = 0;
static AndroidCameraCapture *imageCapture = 0;


// Helper method to create C string copy
static char* MakeStringCopy (const char* val)
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
	   void initializeLicenseManager(JNIEnv* env, jobject obj, const char *licenseKeyFileName, void (*alertFunction)(const char*) = 0);
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
	static char *pixelData = 0;
	pthread_mutex_t writeFrame_mutex;
	pthread_mutex_t grabFrame_mutex;
	
	JavaVM* _vm = 0; 
	JNIEnv* jni_env = 0;
	jobject obj_Activity;
	
	void AlertCallback(const char* warningMessage)
	{
		jclass dataClass = jni_env->GetObjectClass(obj_Activity);
		if (jni_env->ExceptionCheck())
				jni_env->ExceptionClear();
		if (dataClass != NULL)
		{
			jclass javaClassRef = (jclass) jni_env->NewGlobalRef(dataClass);
			jmethodID javaMethodRef = jni_env->GetMethodID(javaClassRef, "AlertDialogFunction", "(Ljava/lang/String;)V");
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

	
	void _initTracker (char* configuration, char* license)
	{		
		jni_env = 0;
		_vm->AttachCurrentThread(&jni_env, 0);
		jclass unity = jni_env->FindClass("com/unity3d/player/UnityPlayer");
		jfieldID fid_Activity	= jni_env->GetStaticFieldID(unity, "currentActivity", "Landroid/app/Activity;");
		obj_Activity	= jni_env->GetStaticObjectField(unity, fid_Activity);
		initializeLicenseManager(jni_env, obj_Activity, license, AlertCallback);
		
		if (m_Tracker)
			_releaseTracker();
		
		m_Tracker = new VisageTracker(configuration);
		pthread_mutex_init(&writeFrame_mutex, NULL);
		pthread_mutex_init(&grabFrame_mutex, NULL);
		
	}
	
	void _releaseTracker()
	{
		delete m_Tracker;
		m_Tracker = 0;
	}
	
	void Java_app_specta_inc_CameraActivity_WriteFrame(JNIEnv *env, jobject obj, jbyteArray frame)
	{
		LOGI("Java_app_specta_inc_CameraActivity_WriteFrame - called");
		pthread_mutex_lock(&writeFrame_mutex);
		if (!parametersChanged){
			jbyte *pixelData = env->GetByteArrayElements(frame, 0);
			LOGI("Java_app_specta_inc_CameraActivity_WriteFrame: %d | %d | %d | %d" , pixelData[11],pixelData[22],pixelData[33],pixelData[44]);

			imageCapture->WriteFrameYUV((unsigned char*)pixelData);
			env->ReleaseByteArrayElements(frame, pixelData, 0);
		}
		parametersChanged = false;
		LOGI("Java_app_specta_inc_CameraActivity_WriteFrame - END");
		pthread_mutex_unlock(&writeFrame_mutex);
	}

	void Java_app_specta_inc_CameraActivity_setParameters(JNIEnv *env, jobject obj, jint orientation, jint width, jint height, jint flip)
	{
		pthread_mutex_lock(&grabFrame_mutex);
		pthread_mutex_lock(&writeFrame_mutex);
		if (width !=-1)
			camWidth = width;
		if (height !=-1)
			camHeight = height;
		if (orientation !=-1)
			camOrientation = orientation;
		if (flip !=-1)
			camFlip = flip;

		delete imageCapture;
		imageCapture = new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);
		
		
		if (camOrientation == 90 || camOrientation == 270)
		{
			xTexScale =  camHeight / (float) GetNearestPow2(camHeight);
			yTexScale = camWidth / (float) GetNearestPow2(camWidth);
		}
		else
		{
			xTexScale =  camWidth / (float) GetNearestPow2(camWidth);
			yTexScale = camHeight / (float) GetNearestPow2(camHeight);
		}
		
		parametersChanged = true;	
		pthread_mutex_unlock(&writeFrame_mutex);
		pthread_mutex_unlock(&grabFrame_mutex);

        LOGI("Java_app_specta_inc_CameraActivity_setParameters");
	}

	void Java_com_visagetechnologies_facialanimationdemo_CameraActivity_WriteFrame(JNIEnv *env, jobject obj, jbyteArray frame) 
	{
		LOGI("Java_com_visagetechnologies_facialanimationdemo_CameraActivity_WriteFrame - called");
		pthread_mutex_lock(&writeFrame_mutex);
		if (!parametersChanged){
			jbyte *pixelData = env->GetByteArrayElements(frame, 0);

		    LOGI("Java_com_visagetechnologies_facialanimationdemo_CameraActivity_WriteFrame: %d | %d | %d | %d" , pixelData[11],pixelData[22],pixelData[33],pixelData[44]);

			imageCapture->WriteFrameYUV((unsigned char*)pixelData);
			env->ReleaseByteArrayElements(frame, pixelData, 0);
		}
		parametersChanged = false;
		LOGI("Java_com_visagetechnologies_facialanimationdemo_CameraActivity_WriteFrame - END");
		pthread_mutex_unlock(&writeFrame_mutex);

	}

	void Java_com_visagetechnologies_facialanimationdemo_CameraActivity_setParameters(JNIEnv *env, jobject obj, jint orientation, jint width, jint height, jint flip) 
	{
		pthread_mutex_lock(&grabFrame_mutex);
		pthread_mutex_lock(&writeFrame_mutex);
		if (width !=-1)
			camWidth = width;
		if (height !=-1)
			camHeight = height;
		if (orientation !=-1)
			camOrientation = orientation;
		if (flip !=-1)
			camFlip = flip;
		
		

		delete imageCapture;
		imageCapture = new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);
		
		
		if (camOrientation == 90 || camOrientation == 270)
		{
			xTexScale =  camHeight / (float) GetNearestPow2(camHeight);
			yTexScale = camWidth / (float) GetNearestPow2(camWidth);
		}
		else
		{
			xTexScale =  camWidth / (float) GetNearestPow2(camWidth);
			yTexScale = camHeight / (float) GetNearestPow2(camHeight);
		}
		
		parametersChanged = true;	
		pthread_mutex_unlock(&writeFrame_mutex);
		pthread_mutex_unlock(&grabFrame_mutex);
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
	
	void _grabFrame()
	{
		if (imageCapture == 0)
			return;
		pthread_mutex_lock(&grabFrame_mutex);
		long ts;
		pixelData = (char*)imageCapture->GrabFrame(ts);

        LOGI("_grabFrame: %d | %d | %d | %d" , pixelData[11],pixelData[22],pixelData[33],pixelData[44]);
		pthread_mutex_unlock(&grabFrame_mutex);
	}
	
	int _track()
	{
		pthread_mutex_lock(&grabFrame_mutex);

		if (camOrientation == 90 || camOrientation == 270)
			trackingStatus = m_Tracker->track(camHeight, camWidth, pixelData, trackingData, VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);
		else
			trackingStatus = m_Tracker->track(camWidth, camHeight, pixelData, trackingData, VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1);
		
		if (trackingStatus[0] == TRACK_STAT_OFF)
			pixelData = 0;
		LOGI("# Native _track:%d", trackingStatus[0]);
		pthread_mutex_unlock(&grabFrame_mutex);
		return trackingStatus[0];
	}

	const char* _get3DData(float* tx, float* ty, float* tz,float* rx, float* ry, float* rz)
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
		*rx = trackingData[0].faceRotation[0]*180/3.14f;
		*ry = -(trackingData[0].faceRotation[1]+3.14f)*180.0f/3.14f;
		*rz = -trackingData[0].faceRotation[2]*180/3.14f;
		
		
		
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
		sprintf(message, " %4.1f FPS  Status: %s\n Head position %+5.1f %+5.1f %+5.1f \n Rotation (deg) %+5.1f %+5.1f %+5.1f \n\n",
				trackingData[0].frameRate,
				tstatus,
				trackingData[0].faceTranslation[0],
				trackingData[0].faceTranslation[1],
				trackingData[0].faceTranslation[2],
				trackingData[0].faceRotation[0],
				trackingData[0].faceRotation[1],
				trackingData[0].faceRotation[2]);
		
		return MakeStringCopy(message);
	}

	void _bindTexture(int texID)
	{
		// binds a texture with the given native hardware texture id through opengl	
		if (texID != -1 && pixelData != 0)
		{
			glBindTexture(GL_TEXTURE_2D, texID);
			if (camOrientation == 90 || camOrientation == 270)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camHeight, camWidth, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
			else
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);

			LOGI("_bindTexture : %d | %d | %d | %d" , pixelData[11],pixelData[22],pixelData[33],pixelData[44]);
		}
	}

	// a helper function to get all the needed info in one native call
	bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord)
	{
		if (trackingStatus[0] == TRACK_STAT_OFF)
			return false; 
		
		// get vertex number
		*vertexNumber = trackingData[0].faceModelVertexCount;
		
		// get vertices
		memcpy(vertices, trackingData[0].faceModelVertices, 3*(*vertexNumber)*sizeof(float));
		
		// get triangle number
		*triangleNumber = trackingData[0].faceModelTriangleCount;
		
		// get triangles in reverse order
		for(int i = 0; i < *triangleNumber * 3; i++)
		{
			triangles[*triangleNumber * 3 - 1 - i] = trackingData[0].faceModelTriangles[i];
		}
		
		//texture coordinates are normalized to frame
		//and because frame is only in the part of our texture
		//we must scale texture coordinates to match
		//frame_width/tex_width and frame_height/tex_height
		//also x-coord must be flipped
		for (int i = 0; i < *vertexNumber*2; i+=2) {
			texCoord[i+0] = (1.0f - trackingData[0].faceModelTextureCoords[i+0]) * xTexScale;
			texCoord[i+1] = trackingData[0].faceModelTextureCoords[i+1] * yTexScale;
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
			positions[i*2] = position[0];
			positions[i*2 + 1] = position[1];
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
	
}