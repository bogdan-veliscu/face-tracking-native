#include <jni.h>
#include <EGL/egl.h> 
#include <GLES/gl.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include "VisageTracker.h"
#include "VisageFaceAnalyser.h"
#include "VisageRendering.h"
#include "AndroidImageCapture.h"
#include "AndroidCameraCapture.h"
#include "WrapperOpenCV.h"
#include "cv.h"
#include "highgui.h"

#include <android/log.h>
#define  LOG_TAG    "TrackerWrapper"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using namespace std;
using namespace VisageSDK;

static AndroidImageCapture *a_cap_image = 0;
static AndroidCameraCapture *a_cap_camera = 0;
static VsImage *drawImage = 0;
static VsImage *copyImage = 0;

static float m_fps = 0;

void Sleep(int ms) {usleep(ms*1000);}

// neccessary prototype declaration for licensing
namespace VisageSDK
{
void initializeLicenseManager(JNIEnv* env, jobject obj, const char *licenseKeyFileName, void (*alertFunction)(const char*) = 0);
}

/** \file AndroidWrapper.cpp
 * Implementation of simple interface around visage|SDK VisageTracker functionality.
 *
 * In order for Android application, which uses Java as its primary programming language, to communicate with visage|SDK functionality, 
 * which uses C++ as its primary language it is necessary to use Java Native Interface as a framework between the two.  
 *
 * Key members of wrapper are:
 * - m_Tracker: the VisageTracker object
 * - trackingData: the TrackingData object used for retrieving and holding tracking data
 * - displayTrackingResults: method that demonstrates how to acquire, use and display tracking data and 3D face model
 * 
 */

extern "C" {

const int MAX_FACES = 4;

static VisageTracker *m_Tracker = 0;
static VisageFaceAnalyser* m_FaceAnalizer = 0;
static VsImage *m_Frame = 0;

static FaceData trackingData[MAX_FACES];
static FaceData drawTrackingData[MAX_FACES];
static FaceData copyTrackingData[MAX_FACES];
int *trackingStatus = 0;
int drawTrackingStatus[MAX_FACES];
int copyTrackingStatus[MAX_FACES];

VsImage* logo = 0;

bool trackingOk = false;



GLuint texIds[3];

/**
* Texture ID for displaying frames from the tracker.
*/
GLuint frameTexId = 0;

GLuint instructionsTexId = 0;

/**
* Texture coordinates for displaying frames from the tracker.
*/
float xTexCoord;

/**
* Texture coordinates for displaying frames from the tracker.
*/
float yTexCoord;

/**
* Size of the texture for displaying frames from the tracker.
*/
int xTexSize;

/**
* Size of the texture for displaying frames from the tracker.
*/
int yTexSize;

/**
* Aspect of the video.
*/
float videoAspect;

/**
* Size of the OpenGL view.
*/
int glWidth;

/**
* Size of the OpenGL view.
*/
int glHeight;

/**
* Switch that indicates that method for displaying results is called for the first time. 
*/
bool displayed = false;

float displayFramerate = 0;
bool isTracking = false;
int timestamp = 0;

int trackingTime;

int camOrientation;
int camHeight;
int camWidth;
int camFlip;
bool trackerStopped;
pthread_mutex_t displayRes_mutex;
pthread_mutex_t guardFrame_mutex;
VsImage *last_img = 0;
JNIEnv* _env;
jobject _obj;


void AlertCallback(const char* warningMessage)
{
	jclass dataClass = _env->FindClass("com/visagetechnologies/visagetrackerdemo/TrackerActivity");
	if (_env->ExceptionCheck())
			_env->ExceptionClear();
	if (dataClass != NULL)
	{
		jclass javaClassRef = (jclass) _env->NewGlobalRef(dataClass);
		jmethodID javaMethodRef = _env->GetMethodID(javaClassRef, "AlertDialogFunction", "(Ljava/lang/String;)V");
		if (_env->ExceptionCheck())
			_env->ExceptionClear();
		jstring message = _env->NewStringUTF(warningMessage);
		if (javaMethodRef != 0)
			_env->CallVoidMethod(_obj, javaMethodRef, message);

		_env->DeleteGlobalRef(javaClassRef);
		_env->DeleteLocalRef(message);
	}
}

long getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return (long) ((now.tv_sec*1000000000LL + now.tv_nsec)/1000000LL);
}

IplImage* loadLogo(std::string logoPath)
{
	//load logo image
	IplImage* originalLogo = cvLoadImage(logoPath.c_str(), CV_LOAD_IMAGE_UNCHANGED);
	cvCvtColor(originalLogo, originalLogo, CV_BGRA2RGBA);
	return originalLogo;
}


/**
 * Method for initializing the tracker.
 *
 * This method creates a new VisageTracker objects and initializes the tracker.
 */
void Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_TrackerInit(JNIEnv *env, jobject obj, jstring configFilename, jstring appPath)
{
	_env = env;
	_obj = obj;
	const char *_configFilename = env->GetStringUTFChars(configFilename, 0);
	trackerStopped = false;
	displayed = false;

	//initialize licensing
	//example how to initialize license key
	initializeLicenseManager(env, obj, "/data/data/com.visagetechnologies.visagetrackerdemo/files/license-key-name.vlc", AlertCallback);

    logo = (VsImage*)loadLogo("/data/data/com.visagetechnologies.visagetrackerdemo/files/logo.png");
    if (!logo)
               LOGE("Logo was not successfully loaded");

	pthread_mutex_destroy(&displayRes_mutex);
	pthread_mutex_init(&displayRes_mutex, NULL);

	pthread_mutex_destroy(&guardFrame_mutex);
    pthread_mutex_init(&guardFrame_mutex, NULL);

	// delete previously allocated objects
	delete a_cap_camera;
	a_cap_camera = 0;

	a_cap_image = 0;
	m_Tracker = new VisageTracker(_configFilename);
    m_Frame = vsCreateImage(vsSize(camWidth, camHeight), 8, 3);
    if (m_FaceAnalizer){
         delete m_FaceAnalizer;
      }
     m_FaceAnalizer = new VisageFaceAnalyser();

        const char *_appPath = env->GetStringUTFChars(appPath, 0);
        std::string path(_appPath);
        path.append("/bdtsdata/LBF/vfadata");

        //       /data/user/0/com.visagetechnologies.visagetrackerdemo/files/bdtsdata/
        //       /data/user/0/com.visagetechnologies.visagetrackerdemo/files/bdtsdata

    int ret = m_FaceAnalizer->init(path.c_str());
    LOGI("### VisageFaceAnalyser _initFaceAnalyser :%d -> %s", ret, path.c_str());

	LOGI("%s", _configFilename);
	env->ReleaseStringUTFChars(configFilename, _configFilename);
}

/**
 * Method for tracking in image
 */
void Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_TrackFromImage(JNIEnv *env, jobject obj, jint width, jint height )
{
	while(!trackerStopped)
	{
		if (m_Tracker && a_cap_image && !trackerStopped)
		{
			pthread_mutex_lock(&guardFrame_mutex);
			long ts;
            drawImage = a_cap_image->GrabFrame(ts);
			if (trackerStopped || drawImage == 0)
			{
				pthread_mutex_unlock(&guardFrame_mutex);
				return;
			}
			long startTime = getTimeNsec();
			trackingStatus = m_Tracker->track(width, height,  (const char*)drawImage->imageData, trackingData, VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1, MAX_FACES);
			long endTime = getTimeNsec();
			trackingTime = (int)(endTime - startTime);
			pthread_mutex_unlock(&guardFrame_mutex);
			pthread_mutex_lock(&displayRes_mutex);
			for (int i=0; i<MAX_FACES; i++)
            {
                if (trackingStatus[i] == TRACK_STAT_OFF)
                    continue;
                std::swap(drawTrackingData[i], trackingData[i]);
                drawTrackingStatus[i] = trackingStatus[i];
                trackingOk = true;

            }
            isTracking = true;
            pthread_mutex_unlock(&displayRes_mutex);
		}
		else
			Sleep(100);

			}
	return;
}

/**
 * Method that sets frame parameters for tracking from camera
 */
int Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_setParameters(JNIEnv *env, jobject obj, jint width, jint height, jint orientation, jint flip)
{
	camOrientation = orientation;
	camHeight = height;
	camWidth = width;
	camFlip = flip;

	drawImage = vsCreateImage(vsSize(height, width), VS_DEPTH_8U, 3);

	return 0;
}

/**
 * Method for tracking from camera
 */
void Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_TrackFromCam(JNIEnv *env, jobject obj)
{
	while (!trackerStopped)
	{
		if (m_Tracker && a_cap_camera && !trackerStopped)
		{
			pthread_mutex_lock(&guardFrame_mutex);
			long ts;
            VsImage *pixelData = a_cap_camera->GrabFrame(ts);
			if (trackerStopped || pixelData == 0)
            {
                pthread_mutex_unlock(&guardFrame_mutex);
                return;
            }
            long startTime = getTimeNsec();
            if (camOrientation == 90 || camOrientation == 270)
                trackingStatus = m_Tracker->track(camHeight, camWidth, pixelData->imageData, trackingData, VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1, MAX_FACES);
            else
                trackingStatus = m_Tracker->track(camWidth, camHeight, pixelData->imageData, trackingData, VISAGE_FRAMEGRABBER_FMT_RGB, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1, MAX_FACES);
            long endTime = getTimeNsec();
            trackingTime = (int)endTime - startTime;
            LOGI("#### ageRefreshRequested - before");

            //memcpy(m_Frame->imageData, pixelData, (frameSize)*sizeof(char));
             int   detectedAge = m_FaceAnalizer->estimateAge(m_Frame, trackingData);
             int   detectedGender = m_FaceAnalizer->estimateGender(m_Frame, trackingData);

            LOGI("#### ageRefreshRequested - after : %d --> %d", detectedAge, detectedGender);

			pthread_mutex_unlock(&guardFrame_mutex);
			pthread_mutex_lock(&displayRes_mutex);
			for (int i=0; i<MAX_FACES; i++)
			{
			    if (trackingStatus[i] == TRACK_STAT_OFF)
			        continue;
                std::swap(drawTrackingData[i], trackingData[i]);
                drawTrackingStatus[i] = trackingStatus[i];
                trackingOk = true;

            }
			isTracking = true;

            if (trackingOk)
                vsCopy(pixelData, drawImage);

			pthread_mutex_unlock(&displayRes_mutex);
		}
		else
			Sleep(1);
	}
	return;
}


/**
 * Stops the tracker.
 */
void Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_TrackerStop(JNIEnv *env, jobject obj)
{
	if (m_Tracker)
	{
		trackerStopped = true;
		trackingOk =false;
		pthread_mutex_lock(&guardFrame_mutex);
		for (int i=0; i<MAX_FACES; i++)
            {
                copyTrackingStatus[i] = TRACK_STAT_OFF;
                drawTrackingStatus[i] = TRACK_STAT_OFF;
                trackingStatus[i] = TRACK_STAT_OFF;
            }
		m_Tracker->stop();
		delete m_Tracker;
		m_Tracker = 0;
		vsReleaseImage(&drawImage);
		drawImage = 0;
		vsReleaseImage(&copyImage);
		copyImage = 0;
        VisageRendering::Reset();
		pthread_mutex_unlock(&guardFrame_mutex);

	}
}


/**
* Writes raw image data into @ref VisageSDK::AndroidImageCapture object. VisageTracker reads this image and performs tracking.
* @param frame byte array with image data
* @param width image width
* @param height image height
*/
void Java_com_visagetechnologies_visagetrackerdemo_ImageTrackerView_WriteFrameImage(JNIEnv *env, jobject obj, jbyteArray frame, jint width, jint height) 
{
	if (!a_cap_image)
		a_cap_image = new AndroidImageCapture(width, height, VISAGE_FRAMEGRABBER_FMT_RGB);
	jbyte *f = env->GetByteArrayElements(frame, 0);
	a_cap_image->WriteFrame((unsigned char *)f, (int)width, (int)height);
	env->ReleaseByteArrayElements(frame, f, 0);
}


/**
* Writes raw image data into @ref VisageSDK::AndroidCameraCapture object. VisageTracker reads this image and performs tracking. User should call this 
* function whenever new frame from camera is available. Data inside frame should be in Android NV21 (YUV420sp) format and @ref VisageSDK::AndroidCameraCapture
* will perform conversion to RGB.
* @param frame byte array with image data
*/
void Java_com_visagetechnologies_visagetrackerdemo_JavaCamTrackerView_WriteFrameCamera(JNIEnv *env, jobject obj, jbyteArray frame) 
{
	if (trackerStopped)
		return;
	if (!a_cap_camera)
		a_cap_camera = new AndroidCameraCapture(camWidth, camHeight, camOrientation, camFlip);
	jbyte *f = env->GetByteArrayElements(frame, 0);
	a_cap_camera->WriteFrameYUV((unsigned char *)f);
	env->ReleaseByteArrayElements(frame, f, 0);
}


/**
 * Method for displaying tracking results.
 *
 * This method is periodically called by the application rendering thread to get and display tracking results. The results are retrieved using VisageSDK::TrackingData structure and displayed OpenGL ES for visual data (frames from camera and 3D face model). It shows how to properly interpret tracking data and setup the OpenGL scene to display 3D face model retrieved from the tracker correctly aligned to the video frame. 
 *
 * @param width width of GLSurfaceView used for rendering.
 * @param height height of GLSurfaceView used for rendering.
 */
bool Java_com_visagetechnologies_visagetrackerdemo_TrackerRenderer_displayTrackingStatus(JNIEnv* env, jobject obj, jint width, jint height)
{
	if(!displayed)
	{
		glWidth = width;
		glHeight = height;
		displayed = true;
	}
	if (!m_Tracker || trackerStopped || !isTracking || !drawImage)
		return false;

    videoAspect = drawImage->width / (float) drawImage->height;

	if (!copyImage)
    {
        copyImage = vsCloneImage(drawImage);
    }

    pthread_mutex_lock(&displayRes_mutex);
    vsCopy(drawImage, copyImage);

    for (int i=0; i<MAX_FACES; i++)
    {
        if (drawTrackingStatus[i] == TRACK_STAT_OFF)
            continue;
        copyTrackingData[i] = drawTrackingData[i];
        copyTrackingStatus[i] = drawTrackingStatus[i];
    }
    pthread_mutex_unlock(&displayRes_mutex);
    float tmp;
        if(drawImage->width < drawImage->height)
        {
    		tmp = glHeight;
        	glHeight = glWidth / videoAspect;
    		if (glHeight > tmp)
    		{
    			glWidth  = glWidth*tmp/glHeight;
    			glHeight = tmp;
    		}
        }
        else
        {
    		tmp = glWidth;
        	glWidth = glHeight * videoAspect;
    		if (glWidth > tmp)
    		{
    			glHeight  = glHeight*tmp/glWidth;
    			glWidth = tmp;
    		}
        }

    VisageRendering::DisplayResults(&copyTrackingData[0], copyTrackingStatus[0], glWidth, glHeight, copyImage, DISPLAY_FRAME);
    VisageRendering::DisplayLogo(logo, glWidth, glHeight);

    for (int i=0; i<MAX_FACES; i++)
    {
        if(copyTrackingStatus[i] == TRACK_STAT_OK)
        		VisageRendering::DisplayResults(&copyTrackingData[i], copyTrackingStatus[i], glWidth, glHeight, copyImage, DISPLAY_DEFAULT - DISPLAY_FRAME);
    }

	return true;
}



/**
* Method for getting frame rate information from the tracker.
*
* @return float value of frame rate obtained from the tracker.
*/
float Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_GetFps(JNIEnv* env, jobject obj)
{
	return trackingData[0].frameRate;
}

/**
* Method for getting frame rate information from the tracker.
*
* @return float value of frame rate obtained from the tracker.
*/
int Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_GetTrackTime(JNIEnv* env, jobject obj)
{
	return trackingTime;
}


/**
* Method for getting the tracking status information from tracker.
*
* @return tracking status information as string.
*/
JNIEXPORT jstring JNICALL Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_GetStatus(JNIEnv* env, jobject obj)
{
	char* msg;

    for (int i=0; i<MAX_FACES; i++)
    {
        if (drawTrackingStatus[i] == TRACK_STAT_OK)
            return env->NewStringUTF("OK");
    }

    for (int i=0; i<MAX_FACES; i++)
    {
        if (drawTrackingStatus[i] == TRACK_STAT_RECOVERING)
            return env->NewStringUTF("RECOVERING");
    }

    for (int i=0; i<MAX_FACES; i++)
    {
        if (drawTrackingStatus[i] == TRACK_STAT_INIT)
            return env->NewStringUTF("INITIALIZING");
    }

    return env->NewStringUTF("OFF");
}

float Java_com_visagetechnologies_visagetrackerdemo_TrackerRenderer_getTrackerFps( JNIEnv*  env )
{
	return m_fps;
}


float Java_com_visagetechnologies_visagetrackerdemo_TrackerActivity_GetDisplayFps(JNIEnv* env)
{
	return displayFramerate;
}


bool Java_com_visagetechnologies_visagetrackerdemo_TrackerView_IsAutoStopped(JNIEnv* env)
{
	//RESET VARIABLES IN WRAPPER
	if (m_Tracker)
   {

        for (int i=0; i<MAX_FACES; i++)
        {
            if (trackingStatus[i] != TRACK_STAT_OK)
                return true;
        }
        return false;

   }
   else
        return false;
}

}
