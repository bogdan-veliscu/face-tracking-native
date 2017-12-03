#include <jni.h>
#include <EGL/egl.h> 
#include <GLES/gl.h>
#include <vector>
#include <stdio.h>
#include "VisageDrawing.h"
#include "VisageFeaturesDetector.h"
#include "WrapperOpenCV.h"
#include "cv.h"
#include "Common.h"
#include "highgui.h"
#include "VisageFaceRecognition.h"
#include "VisageFaceAnalyser.h"

#include <android/log.h>
#define  LOG_TAG    "DetectorWrapper"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define MAX_SIZE 768

using namespace std;
using namespace VisageSDK;

// neccessary prototype declaration for licensing
namespace VisageSDK
{
	void initializeLicenseManager(JNIEnv* env, jobject obj, const char *licenseKeyFileName, void (*alertFunction)(const char*) = 0);
}

/** \file VisageFeaturesDetectorWrapper.cpp
 * Implementation of simple interface around visage|SDK VisageFeaturesDetector functionality.
 *
 * In order for Android application, which uses Java as its primary programming language, to communicate with visage|SDK functionality, 
 * which uses C++ as its primary language it is necessary to use Java Native Interface as a framework between the two.  
 *
 * Key members of wrapper are:
 * - visageFeaturesDetector: the VisageFeaturesDetector object
 * - displayResults: method that demonstrates how to display data obtained from VisageFeaturesDetector
 * 
 */

extern "C" {

VisageFeaturesDetector *visageFeaturesDetector = 0;
VisageFaceRecognition *visageFaceRecognition = 0;
VisageFaceAnalyser *visageFaceAnalyser = 0;

/**
* Texture ID for displaying frames.
*/
extern GLuint frameTexId;
extern GLuint logoTexId;

extern IplImage *_drawImage;
extern IplImage *_logoImage;

extern bool _isDisplayed;

IplImage* input = 0;
FDP* fdp = 0;

int maxFaces = 100;
int faces = -1;
FaceData* data;
int choose_feature;

static VsImage* logo = 0;

static IplImage* loadLogo(std::string logoPath)
{
	IplImage* originalLogo = cvLoadImage(logoPath.c_str(), CV_LOAD_IMAGE_UNCHANGED);
	cvCvtColor(originalLogo, originalLogo, CV_BGRA2RGBA);

	if (!originalLogo)
		return NULL;

	return originalLogo;
}

/**
 * Method for initializing the detector.
 *
 * This method creates a new VisageFeaturesDetector object and initializes it.
 * @param appPath path to root folder where Android application data is stored
 * @param modelName name of generic Active Appearance Model file used for detection
 */

void Java_com_visagetechnologies_facedetectdemo_DetectorActivity_DetectorInit(JNIEnv *env, jobject obj, jstring appPath, jstring modelName, jint chooseFeature)
{
	
	//initialize licensing
	//example how to initialize license key
	initializeLicenseManager(env, obj, "/data/data/com.visagetechnologies.facedetectdemo/files/770-007-786-963-154-432-507-591-632-107-595.vlc");
	
	if(visageFeaturesDetector == 0)
	{
		const char *_modelName = env->GetStringUTFChars(modelName, 0);
		const char *_appPath = env->GetStringUTFChars(appPath, 0);

		std::string path(_appPath);
		path.append("/bdtsdata");

		visageFeaturesDetector = new VisageFeaturesDetector();
		bool ret = visageFeaturesDetector->Initialize(path.c_str());
		
		env->ReleaseStringUTFChars(modelName, _modelName);
		env->ReleaseStringUTFChars(appPath, _appPath);

		data = 0;
	}
	choose_feature = chooseFeature;

	visageFaceAnalyser = new VisageFaceAnalyser();
	const char *dataPath = "/data/data/com.visagetechnologies.facedetectdemo/files/bdtsdata/LBF/vfadata";
    bool ret = visageFaceAnalyser->init(dataPath);
    if (ret)
        LOGE("FaceAnalysis successfully initialized");
    else
        LOGE("Error while initializing FaceAnalysis");
	_isDisplayed = false;

	logo = (VsImage*)loadLogo("/data/data/com.visagetechnologies.facedetectdemo/files/logo.png");
    if (!logo)
        LOGE("Logo was not successfully loaded");
}

/**
* Method for passing raw image data to detector and initiating detection of features in it. Detection results are returned through FDP object. By calling static method DrawFDP of Common class detection results are drawn directly on top of image used for detection.
* @param frame raw image data of image used for detection.
* @param width width of image used for detection.
* @param height height of image used for detection.
*/
void Java_com_visagetechnologies_facedetectdemo_ImageDetectorView_DetectFeatures(JNIEnv *env, jobject obj, jbyteArray frame, jint width, jint height)
{
	if(visageFeaturesDetector == 0)
	{
		return;
	}

	jbyte *f = env->GetByteArrayElements(frame, 0);
	input = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	memcpy(input->imageData, (unsigned char*) f, input->imageSize);

	if(data != 0)
	{
		delete[] data;
	}

	data = new FaceData[maxFaces];
	faces = visageFeaturesDetector->detectFacialFeatures((VsImage*)input, data, maxFaces);

	env->ReleaseByteArrayElements(frame, f, 0);
}

static bool wasInited = false;

/**
 * Method for displaying detection results.
 */
void Java_com_visagetechnologies_facedetectdemo_DetectorRenderer_displayResults(JNIEnv* env, jobject obj)
{
    if(faces > 0)
    {
        for (int i = 0; i<faces; i++)
        {
            if (choose_feature == 0)
                VisageDrawing::drawResults((VsImage*)input, &data[i]);
            else
            {
                int age = visageFaceAnalyser->estimateAge((VsImage*)input, &data[i]);
                if (age > -1)
                    VisageDrawing::drawAge(input, data[i], age);
                int gender = visageFaceAnalyser->estimateGender((VsImage*)input, &data[i]);

                if (gender > -1)
                    VisageDrawing::drawGender(input, data[i], gender);
                //estimate emotions on biggest face in image
                int biggestFaceIndex = 0;
                float biggestFaceScale = 0;
                for (int i = 0; i < faces; i++)
                {
                    if(data[i].faceScale > biggestFaceScale)
                    {
                        biggestFaceScale = data[i].faceScale;
                        biggestFaceIndex = i;
                    }
                }
                float emotion_prob_estimates[7]={0.0f};
                int emotionsEstimated = visageFaceAnalyser->estimateEmotion((VsImage*)input, &data[biggestFaceIndex], emotion_prob_estimates);
                if (emotionsEstimated)
                    VisageDrawing::drawEmotions(input, data[biggestFaceIndex], emotion_prob_estimates);
            }
        }
    }

    _drawImage = input;
    _logoImage = (IplImage*)logo;

}

/**
 * Method for cleaning up resources used by application.
 */
void Java_com_visagetechnologies_facedetectdemo_DetectorActivity_CleanUp(JNIEnv* env, jobject obj)
{
	cvReleaseImage(&input);
	delete[] data;
	data = 0;
	frameTexId = 0;
	logoTexId = 0;
	faces = -1;
}

}
