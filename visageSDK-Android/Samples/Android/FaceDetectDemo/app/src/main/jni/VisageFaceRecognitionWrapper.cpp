#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <vector>
#include <stdio.h>
#include "VisageDrawing.h"
#include "VisageFeaturesDetector.h"
#include "Common.h"
#include "WrapperOpenCV.h"
#include "cv.h"
#include "highgui.h"
#include "VisageFaceRecognition.h"
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#define  LOG_TAG    "DetectorWrapper"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define MAX_SIZE 768

#define PATH_TO_GALLERY "/Famous Actors/gallery"
#define MAX_FACES 3
#define TOP_N_FACES 3
#define SIMILARITY_THRESHOLD 0.55f

using namespace std;
using namespace VisageSDK;

// neccessary prototype declaration for licensing
namespace VisageSDK
{
    void initializeLicenseManager(JNIEnv* env, jobject obj, const char *licenseKeyFileName, void (*alertFunction)(const char*) = 0);
}


/** \file VisageFaceRecognitionWrapper.cpp
 * Implementation of simple interface around visage|SDK VisageFaceRecogntion functionality.
 *
 * In order for Android application, which uses Java as its primary programming language, to communicate with visage|SDK functionality,
 * which uses C++ as its primary language it is necessary to use Java Native Interface as a framework between the two.
 *
 * Key members of wrapper are:
 * - VisageFaceRecogntion: the VisageFaceRecogntion object
 * - displayIdentity: method that demonstrates how to display data obtained from VisageFaceRecognition
 *
 */

extern "C" {

/**
* Texture ID for displaying frames.
*/
extern GLuint frameTexId;
extern GLuint logoTexId;

GLuint texIdsRec[3];

extern bool _isDisplayed;

extern IplImage *_drawImage;
extern IplImage *_logoImage;

std::string pathToData;

AAssetManager *asset_manager;

IplImage* inputImage = 0;
bool galleryCreated = false;

int facesRec = -1;

FaceData *dataRec = 0;

float sim[TOP_N_FACES];

char result[1024];

VisageFeaturesDetector *m_visageFeaturesDetector = 0;
VisageFaceRecognition *m_visageFaceRecognition = 0;

static VsImage* logo = 0;

static IplImage* loadLogo(std::string logoPath)
{
	//load logo image
	IplImage* originalLogo = cvLoadImage(logoPath.c_str(), CV_LOAD_IMAGE_UNCHANGED);
    cvCvtColor(originalLogo, originalLogo, CV_BGRA2RGBA);

    if (!originalLogo)
        return NULL;

    return originalLogo;
}

/**
 * Method which adds descriptors to VisageFaceRecognition gallery.
 *
 * This method goes through all pre-loaded images in the Famous Actors/gallery/ folder and adds corresponding descriptor to VisageFaceRecognition gallery for each.
 * Returns 0 if VisageFeaturesDetector object has not been initialized yet or if the Famous Actors/gallery/ folder is empty.
 */
int createGallery()
{
    if (galleryCreated)
        return 1;
    if (!m_visageFeaturesDetector)
        return 0;

    std::string pathToGallery(pathToData);
    pathToGallery.append(PATH_TO_GALLERY);
    AAssetDir* assetDir = AAssetManager_openDir(asset_manager, (string("trackerdata") + string(PATH_TO_GALLERY)).c_str());
    const char* filename;
    while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL)
    {
       FaceData m_recData;
       IplImage* frame = 0;
       std::string pathToImage = pathToGallery;
       pathToImage.append("/");
       pathToImage.append(filename);
       frame = cvLoadImage(pathToImage.c_str());
       if (!frame)
           LOGE("Frame was not successfully loaded");
       int n_faces = m_visageFeaturesDetector->detectFacialFeatures((VsImage*) frame, &m_recData);
       char name[256] = "";
       char *pos = strchr(filename, '.');
       int offset = (int)(pos - filename);
       strncat(name, filename, offset);
       if(n_faces > 0)
          m_visageFaceRecognition->addDescriptor((VsImage*) frame, &m_recData, (const char*)name);
    }
    galleryCreated = true;

    AAssetDir_close(assetDir);

    return 1;
}

/**
 * Method that loads AAssetManager.
 */
void Java_com_visagetechnologies_facedetectdemo_RecognitionActivity_load(JNIEnv *env, jobject obj, jobject assetManager)
{
    asset_manager = AAssetManager_fromJava(env, assetManager);
}

/**
 * Method for initializing the recognizer.
 *
 * This method creates a new VisageFaceRecognition object and calls method createGallery() for loading VisageFaceRecognition gallery.
 */
void Java_com_visagetechnologies_facedetectdemo_RecognitionActivity_initRecognition(JNIEnv *env, jobject obj, jstring appPath)
{
    initializeLicenseManager(env, obj, "/data/data/com.visagetechnologies.facedetectdemo/files/770-007-786-963-154-432-507-591-632-107-595.vlc");
    const char *_appPath = env->GetStringUTFChars(appPath, 0);
    std::string path(_appPath);
    pathToData = path;
    path.append("/bdtsdata");
    m_visageFeaturesDetector = new VisageFeaturesDetector();
    bool ret = m_visageFeaturesDetector->Initialize(path.c_str());

    path.append("/NN");

    m_visageFaceRecognition = new VisageFaceRecognition(path.c_str());

    if (!(m_visageFaceRecognition->is_initialized))
    {
        LOGI("Could not load required face recognition data files!");
        return;
    }

    int success = createGallery();
    if (!success)
    {
        LOGI("Could not load gallery images!");
        return;
    }

    if(!(m_visageFaceRecognition->is_initialized))
    {
        LOGI("Could not load required face recognition data files!");
        return;
    }

    _isDisplayed = false;

    logo = (VsImage*)loadLogo("/data/data/com.visagetechnologies.facedetectdemo/files/logo.png");
    if (!logo)
        LOGE("Logo was not successfully loaded");
}

/**
 * Method which performs face recognition on a given image.
 *
 * Performs face recognition on a given image and returns the image with the name of the recognized person or the person with the most similar face found in the VisageFaceRecognition gallery.
 */
void Java_com_visagetechnologies_facedetectdemo_ImageDetectorView_recognizeFace(JNIEnv *env, jobject obj, jbyteArray frame, jint width, jint height)
{
    if(m_visageFeaturesDetector == 0)
        return;

    jbyte *f = env->GetByteArrayElements(frame, 0);
    inputImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
    memcpy(inputImage->imageData, (unsigned char*) f, inputImage->imageSize);
    dataRec = new FaceData[MAX_FACES];
	facesRec = m_visageFeaturesDetector->detectFacialFeatures((VsImage*)inputImage, dataRec, MAX_FACES);
	if (facesRec > 0)
	{
        for (int i = 0; i < facesRec; i++)
        {
            if (m_visageFaceRecognition->is_initialized)
            {
                const int DESCRIPTOR_SIZE = m_visageFaceRecognition->getDescriptorSize();
                short *descriptor = new short[DESCRIPTOR_SIZE];
                const char* names[TOP_N_FACES];
                int success = m_visageFaceRecognition->extractDescriptor(dataRec, (VsImage*)inputImage, descriptor);
                if (!success)
                {
                    LOGI("Descriptor was not successfully extracted");
                    return;
                }
                int rec = m_visageFaceRecognition->recognize(descriptor, TOP_N_FACES, names, sim);
                if (!rec)
                    LOGI("Number of faces for comparison needs to be larger than 0");
                sprintf(result, "%s %f", names[0], sim[0]);
            }
        }
	}
	else
	    LOGI("Face was not detected");
}


/**
 * Method for displaying recognition results.
 */
void Java_com_visagetechnologies_facedetectdemo_DetectorRenderer_displayIdentity(JNIEnv* env, jobject obj)
{
    if (facesRec > 0)
    {
        for (int i = 0; i < facesRec; i++)
        {
            if (sim[0] > SIMILARITY_THRESHOLD)
                VisageDrawing::drawMatchingIdentity(inputImage, dataRec[i], result);
            else
                VisageDrawing::drawMatchingIdentity(inputImage, dataRec[i], "?");
        }
    }
    _drawImage = inputImage;
    _logoImage = (IplImage*)logo;
}

/**
 * Method for cleaning up resources used by application.
 */
void Java_com_visagetechnologies_facedetectdemo_RecognitionActivity_CleanUp(JNIEnv* env, jobject obj)
{
    cvReleaseImage(&inputImage);
    inputImage = 0;
    delete[] dataRec;
    delete m_visageFaceRecognition;
    m_visageFaceRecognition = 0;
    delete m_visageFeaturesDetector;
    m_visageFeaturesDetector = 0;
    galleryCreated = false;
    facesRec = -1;
    dataRec = 0;
	frameTexId = 0;
	logoTexId = 0;

}

}

