// FaceDetectorDoc.cpp : implementation of the CFaceDetectorDoc class
//

#include "stdafx.h"
#include "direct.h"
#include <io.h>
#include <math.h>
#include "FaceDetector.h"

#include "FaceDetectorDoc.h"
#include "VisageFeaturesDetector.h"
#include "VisageFaceAnalyser.h"
#include "Common.h"
//#include <vld.h>

#include "VisageFaceRecognition.h"

#include "cv.h"
#include "highgui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFaceDetectorDoc

IMPLEMENT_DYNCREATE(CFaceDetectorDoc, CDocument)

BEGIN_MESSAGE_MAP(CFaceDetectorDoc, CDocument)
	ON_COMMAND(ID_FACIALFEATUREDETECTION_DETECTFACIALFEATURESINIMAGE, &CFaceDetectorDoc::OnFacialfeaturedetectionDetect)
	ON_COMMAND(ID_FACIALFEATUREDETECTION_DETECTFACESINIMAGE, &CFaceDetectorDoc::OnFacesdetectionDetect)
	ON_COMMAND(ID_BATCHPROCESSING_BATCHDETECTIONANDANALYSIS, &CFaceDetectorDoc::OnBatchDetection)
	ON_COMMAND(ID_FACEANALYSER_AGE, &CFaceDetectorDoc::OnAgeEstimation)
	ON_COMMAND(ID_FACEANALYSER_EMOTIONS, &CFaceDetectorDoc::OnEmotionsEstimation)
	ON_COMMAND(ID_FACEANALYSER_GENDER, &CFaceDetectorDoc::OnGenderEstimation)
	ON_COMMAND(ID_FACERECOGNITION, &CFaceDetectorDoc::OnFaceRecognition)
END_MESSAGE_MAP()


static string m_lastRecognitionPath = "";

//logo image
IplImage* logo = NULL;


// CFaceDetectorDoc construction/destruction
/**
* Constructor.
* Initialises the Facial Feature Detector @ref m_Detector
*/
CFaceDetectorDoc::CFaceDetectorDoc()
{
	const char *dataPath = ".";

	m_Detector = new VisageFeaturesDetector();

	bool init = m_Detector->Initialize(dataPath);

	if (!init) {
		MessageBox(0,"Could not load required data files!","Error",MB_ICONERROR);
		delete m_Detector;
		m_Detector = 0;
	}

	m_VFA = NULL;
	m_VFR = NULL;

	VisageFaceRecognition *vfr = new VisageFaceRecognition("bdtsdata/NN/fr.bin");

	//load logo image, working directory is Samples/data
	logo = cvLoadImage("../OpenGL/data/logo.png", CV_LOAD_IMAGE_UNCHANGED);
}

CFaceDetectorDoc::~CFaceDetectorDoc()
{
	delete m_VFA;
	delete m_Detector;

	//
	cvReleaseImage(&logo);
}

BOOL CFaceDetectorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


// CFaceDetectorDoc serialization

void CFaceDetectorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CFaceDetectorDoc diagnostics

#ifdef _DEBUG
void CFaceDetectorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFaceDetectorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

#define MAX_IMAGE_SIZE 1000.0f
#define MIN_IMAGE_SIZE 480.0f
#define MAX_FACES 100

static IplImage* loadAndAdjustImage(char* fileName)
{
	IplImage* frame = 0;
	IplImage* resizedFrame = 0;
	frame = cvLoadImage(fileName);
	if(!frame) 
		return NULL;

	//resize the image if image is too big or too small
	float resizeFactor = 1.0f;
	if(MAX(frame->width, frame->height) > MAX_IMAGE_SIZE)
		resizeFactor = MAX(frame->width, frame->height) / MAX_IMAGE_SIZE;
	else if(MIN(frame->width, frame->height) < MIN_IMAGE_SIZE)
		resizeFactor = MIN(frame->width, frame->height) / MIN_IMAGE_SIZE;
	if(resizeFactor == 1.0f)
		return frame;
	else
	{
		resizedFrame = cvCreateImage(cvSize(cvRound(frame->width / resizeFactor), cvRound(frame->height / resizeFactor)), frame->depth, frame->nChannels);
		cvResize(frame, resizedFrame);
		cvReleaseImage(&frame);
		return resizedFrame;
	}
}

static void featurePointsToFile(const FaceData &faceData, char* fileName, int faceIndex)
{
	char tmpFileName[200];
	strcpy(tmpFileName,fileName);
	if(strlen(tmpFileName)>4)
	{
		tmpFileName[strlen(tmpFileName)-4] = 0;

		char idx[10];
		sprintf(idx, "%d", faceIndex);
		strcat(tmpFileName, "_");
		strcat(tmpFileName, idx);
		strcat(tmpFileName,".fdp");
		faceData.featurePoints2D->saveToFile(tmpFileName); 
	}
}

/**
*	Initializes face analysis module.

Initializes face analysis object.

@return true if successful, false if not.
*
*/
bool CFaceDetectorDoc::initFaceAnalyser()
{
	if(m_VFA)
	{
		return true;
	}
	else
	{
		m_VFA = new VisageFaceAnalyser();
		const char *dataPath = "bdtsdata/LBF/vfadata";
		int is_initialized = m_VFA->init(dataPath);

		if((is_initialized & VFA_AGE) != VFA_AGE)
		{
			MessageBox(0,"Could not load required age data files!", "Error", MB_ICONERROR);
			return false;
		}

		if((is_initialized & VFA_EMOTION) != VFA_EMOTION)
		{
			MessageBox(0,"Could not load required emotion data files!", "Error", MB_ICONERROR);
			return false;
		}

		if((is_initialized & VFA_GENDER) != VFA_GENDER)
		{
			MessageBox(0,"Could not load required gender data files!", "Error", MB_ICONERROR);
			return false;
		}

		return true;
	}
}

/**
*	Initializes face recognition module and loads the gallery.

Initializes face recognition object and creates gallery from the images located in Famous Actors/gallery/ folder.

@return true if successful, false if not. False is returned if the VisageFaceRecognition object cannot be initialized of if the gallery cannot be created.
*
*/
bool CFaceDetectorDoc::initFaceRecognition()
{
	if (m_VFR)
	{
		return true;
	}
	else
	{
		const char *dataPath = "bdtsdata/NN/fr.bin";
		m_VFR = new VisageFaceRecognition(dataPath);

		int success = createGallery("../OpenGL/data/FaceDetector/Famous Actors/gallery");

		if (!success)
		{
			MessageBox(0, "Could not load gallery images!", "Error", MB_ICONERROR);
			return false;
		}

		
		if(!(m_VFR->is_initialized))
		{
			MessageBox(0, "Could not load required face recognition data files!", "Error", MB_ICONERROR);
			return false;
		}

		return true;
	}

}

static void writeUnlicencedMessage(IplImage* img, const char* warning)
{
	CvFont font;
	float scale = img->width / 600.0f;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, scale, scale, 0, 1, CV_AA);
	string str1 = "You are using an unlicensed";
	string str2 = "version of visage|SDK.";
	int top = ((unsigned char)img->imageData[0] + (unsigned char)img->imageData[1] + (unsigned char)img->imageData[2]) % (img->width / 4) + 10;
	cvPutText(img, warning, cvPoint(20, top), &font, cvScalar(255, 0,0)); 
	cvPutText(img, str1.c_str(), cvPoint(20, top+36*scale), &font, cvScalar(255,0,0));
	cvPutText(img, str2.c_str(), cvPoint(20, top+72*scale), &font, cvScalar(255,0,0));
}

/**
*	Performs facial features detection on a file.

	Perform facial feature detection on the image given by fileName. If successful, return the image with the feature points drawn on top of it. Also, an FDP object is created and saved in the current working directory.
	Otherwise, return NULL.
	
	@param fileName The name of the image file to process.
	@return annotated image file if successful, NULL if not.
*
*/
IplImage* CFaceDetectorDoc::DetectFeatures(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);
	if(!image)
		return NULL;

	if (!m_Detector)
		return image;

	FaceData* data = new FaceData[MAX_FACES];
	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES);

	float head_pitch = atan2(data->faceTranslation[1], data->faceTranslation[2]);
	float pitchComp = data->faceRotation[0] - head_pitch;
	float deg_ROT = data->faceRotation[0] * 180.0f / CV_PI;
	float deg_COMP = pitchComp * 180.0f / CV_PI;

	if(n_faces > 0)
	{
		//write feature points detection results to file
		//for(int i = 0; i < n_faces; i++)
		//	featurePointsToFile(data[i], fileName, i);

		//draw feature points detection results for each face to image
		for (int i = 0; i < n_faces; i++) {
			VisageDrawing::drawResults((VsImage*)image, &data[i]);
		}

	}
	else if(n_faces == 0)
	{		
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt="Could not detect face!";
		cvPutText(image, error_txt.c_str(), cvPoint(image->width/14,image->height/14), &outline, CV_RGB(255,255,255));
		cvPutText(image, error_txt.c_str(), cvPoint(image->width/14,image->height/14), &font, CV_RGB(0,0,255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 4);
	cvCvtColor(image, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	// getNormalizedFaceImage usage example
	/*
	if(n_faces > 0)
	{
		IplImage *grayFace = cvCreateImage(cvGetSize(image), image->depth, 1);
		cvCvtColor(image, grayFace, CV_RGB2GRAY);

		IplImage *normFace = cvCreateImage(cvSize(512, 512), grayFace->depth, 1);
		cvSetZero(normFace);

		FDP *normFDP = new FDP;

		m_VFA->getNormalizedFaceImage((VsImage*)grayFace, (VsImage*)normFace, &data[0], normFDP, VS_NORM_POSE | VS_NORM_AU );

		IplImage *normFaceColor = cvCreateImage(cvGetSize(normFace), IPL_DEPTH_8U, 3);

		cvCvtColor(normFace, normFaceColor, CV_GRAY2RGB);

		cvSaveImage("norm_face.png",normFaceColor);
		
		cvReleaseImage(&normFaceColor);
		cvReleaseImage(&normFace);
		cvReleaseImage(&grayFace);
	}
	*/

	cvReleaseImage(&image);

	delete[] data;
	return imageBGRA;
}

/**
*	Performs face detection on a file.

	Perform face detection on the image given by fileName. If successful, return the image with the facial bounding box drawn on top of it.
	Otherwise, return NULL.

	@param fileName The name of the image file to process.
	@return annotated image file if successful, NULL if not.
*
*/
IplImage* CFaceDetectorDoc::DetectFaces(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);
	if (!image)
		return NULL;

	if (!m_Detector)
		return image;

	VsRect* faces = new VsRect[MAX_FACES];
	int n_faces = this->m_Detector->detectFaces((VsImage*)image, faces, MAX_FACES);

	if (n_faces > 0)
	{
		//draw face detection results for each face to image
		for (int i = 0; i < n_faces; i++) {
			cvRectangleR(image, *(CvRect*)&faces[i], CV_RGB(0, 255, 0), 2);
		}

	}
	else if (n_faces == 0)
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt = "Could not detect face!";
		cvPutText(image, error_txt.c_str(), cvPoint(image->width / 14, image->height / 14), &outline, CV_RGB(255, 255, 255));
		cvPutText(image, error_txt.c_str(), cvPoint(image->width / 14, image->height / 14), &font, CV_RGB(0, 0, 255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 4);
	cvCvtColor(image, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);

	delete[] faces;
	return imageBGRA;
}

/**
*	Performs facial features detection on a file.

	Perform facial feature detection altogether with emotions and gender estimation on the image given by fileName. If successful, return the image with the following results drawn on top of it: facial feature points, bars that correspond to the probability of each of estimated facial emotions (anger, disgust, fear, happiness, sadness, surprise) and circle drawn around the face (female - pink circle, male - blue circle). Also, an FDP object is created and saved in the current working directory.
	Otherwise, return NULL.
	
	@param fileName The name of the image file to process.
	@return image file with the results if successful, NULL if not.
*
*/
IplImage* CFaceDetectorDoc::DetectEverything(char *fileName)
{	
	IplImage* image = loadAndAdjustImage(fileName);
	if(!image)
		return NULL;

	if (!m_Detector)
		return image;

	if (!m_VFA)
		return image;

	FaceData* data = new FaceData[MAX_FACES];

	IplImage* outputImage = cvCloneImage(image);

	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES); 

	if(n_faces > 0)
	{
		//estimate emotions only on single-face images
		if(n_faces == 1)
		{
			float emotion_prob_estimates[8]={0.0f};
			int emotionsEstimated = m_VFA->estimateEmotion((VsImage*)image, &data[0], emotion_prob_estimates);
			   if(emotionsEstimated)
				   VisageDrawing::drawEmotions(outputImage, data[0], emotion_prob_estimates);
		}

		for(int i = 0; i < n_faces; i++)
		{
			//estimate gender and draw results for each face on image
			int gender = m_VFA->estimateGender((VsImage*)image, &data[i]);
			if (gender > -1)
				VisageDrawing::drawGender(outputImage, data[i], gender, false);

			//write feature points detection results to file
			//featurePointsToFile(data[i], fileName, i);
		}

		for (int i = 0; i < n_faces; i++)
		{
			//estimate age and draw results for each face on image
			float age = m_VFA->estimateAge((VsImage*)image, &data[i]);
			if (age > -1)
				VisageDrawing::drawAge(outputImage, data[i], age);
		}

		//draw feature points detection results for each face to image
		for (int i = 0; i < n_faces; i++) {
			VisageDrawing::drawResults((VsImage*)outputImage, &data[i]);
		}
	}
	else
	{		
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt="Could not detect face!";
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &outline, CV_RGB(255,255,255));
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &font, CV_RGB(0,0,255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(outputImage->width, outputImage->height), IPL_DEPTH_8U, 4);
	cvCvtColor(outputImage, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);
	cvReleaseImage(&outputImage);

	delete[] data;
	return imageBGRA;
}

/**
*	Performs age estimation on a file.

	Perform age estimation on the image given by fileName. If successful, return the image with age written under the face. 
	If no faces were detected, return the image with adequate message written on it.
	
	@param fileName The name of the image file to process.
	@return annotated image file if successful, image file with adequate message if not.
*
*/
IplImage* CFaceDetectorDoc::EstimateAge(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);
	if(!image)
		return NULL;

	if (!m_Detector)
		return image;

	if(!initFaceAnalyser())
		return image;

	IplImage* outputImage = cvCloneImage(image);

	FaceData* data = new FaceData[MAX_FACES];
	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES); 
	
	if(n_faces > 0)
	{
		for(int i = 0; i < n_faces; i++)
		{
			//estimate age and draw results for each face on image
			float age = m_VFA->estimateAge((VsImage*)image, &data[i]);
			if (age > -1)
				VisageDrawing::drawAge(outputImage, data[i], age);
		}
	}
	else
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt="Could not detect face!";
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &outline, CV_RGB(255,255,255));
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &font, CV_RGB(0,0,255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(outputImage->width, outputImage->height), IPL_DEPTH_8U, 4);
	cvCvtColor(outputImage, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);
	cvReleaseImage(&outputImage);

	delete[] data;
	return imageBGRA;
}

/**
*	Performs emotion estimation on a file.

	Perform emotion estimation on the image given by fileName. If successful, return the image with the bars that correspond the probability of each of estimated facial emotion: anger, disgust, fear, happiness, sadness and surprise.
	If more than one face was detected, return the information about the biggest face. If no faces were detected, return the image with adequate message written on it.
	
	@param fileName The name of the image file to process.
	@return annotated image file if successful, image file with adequate message if not.
*
*/
IplImage* CFaceDetectorDoc::EstimateEmotions(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);
	if(!image)
		return NULL;

	if (!m_Detector)
		return image;

	if(!initFaceAnalyser())
		return image;

	IplImage* outputImage = cvCloneImage(image);

	FaceData* data = new FaceData[MAX_FACES];

	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES); 

	if(n_faces > 0)
	{
		//estimate emotions on biggest face in image
		int biggestFaceIndex = 0;
		float biggestFaceScale = 0;
		for (int i = 0; i < n_faces; i++)
		{
			if(data[i].faceScale > biggestFaceScale)
			{
				biggestFaceScale = data[i].faceScale;
				biggestFaceIndex = i;
			}
		}
		float emotion_prob_estimates[7]={0.0f};
		int emotionsEstimated = m_VFA->estimateEmotion((VsImage*)image, &data[biggestFaceIndex], emotion_prob_estimates);
		if (emotionsEstimated)
			VisageDrawing::drawEmotions(outputImage, data[biggestFaceIndex], emotion_prob_estimates);
		

	}
	else
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt="Could not detect face!";
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &outline, CV_RGB(255,255,255));
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &font, CV_RGB(0,0,255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(outputImage->width, outputImage->height), IPL_DEPTH_8U, 4);
	cvCvtColor(outputImage, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);
	cvReleaseImage(&outputImage);

	delete[] data;
	return imageBGRA;
}

/**
*	Performs gender estimation on a file.

	Perform gender estimation on the image given by fileName. If successful, return the image with the circle drawn around the face. If 
	detected gender is female, the circle is pink and if it is male, the circle is blue.
	If no faces were detected, return the image with adequate message written on it.
	
	@param fileName The name of the image file to process.
	@return annotated image file if successful, image file with adequate message if not.
*
*/
IplImage* CFaceDetectorDoc::EstimateGender(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);
	if(!image)
		return NULL;

	if (!m_Detector)
		return image;

	if(!initFaceAnalyser())
		return image;

	IplImage* outputImage = cvCloneImage(image);

	FaceData* data = new FaceData[MAX_FACES];
	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES); 
	
	if(n_faces > 0)
	{
		for(int i = 0; i < n_faces; i++)
		{
			//estimate gender and draw results for each face on image
			int gender = m_VFA->estimateGender((VsImage*)image, &data[i]);
			if (gender > -1)
				VisageDrawing::drawGender(outputImage, data[i], gender);
		}
	}
	else
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt="Could not detect face!";
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &outline, CV_RGB(255,255,255));
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width/14,outputImage->height/14), &font, CV_RGB(0,0,255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(outputImage->width, outputImage->height), IPL_DEPTH_8U, 4);
	cvCvtColor(outputImage, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);
	cvReleaseImage(&outputImage);

	delete[] data;
	return imageBGRA;
}

/**
*	Performs face recognition on a file.

	Perform face recognition on the image given by filename. Calls helper method initFaceRecognition() which initializes new VisageFaceRecognition object and creates gallery from the images located in Famous Actors/gallery/ folder. Returns the image with the name of the recognized person
	or the name of the person with the most similar face found in the VisageFaceRecognition gallery.
	
	@param fileName The name of the image file to process.
	@return annotated image file.
*
*/
IplImage* CFaceDetectorDoc::RecognizeFace(char *fileName)
{
	IplImage* image = loadAndAdjustImage(fileName);

	if (!image)
		return NULL;

	if (!m_Detector)
		return image;

	if (!initFaceRecognition())
		return image;

	IplImage* outputImage = cvCloneImage(image);

	FaceData* data = new FaceData[MAX_FACES];
	int n_faces = this->m_Detector->detectFacialFeatures((VsImage*)image, data, MAX_FACES);


	if (n_faces > 0)
	{
		for (int i = 0; i < n_faces; i++)
		{
			const int TOP_N_FACES = 3;
			const float SIMILARITY_THRESHOLD = 0.45f;
			const int DESCRIPTOR_SIZE = m_VFR->getDescriptorSize();
			short* descriptor = new short[DESCRIPTOR_SIZE];
			float sim[TOP_N_FACES];
			const char *names[TOP_N_FACES];

			m_VFR->extractDescriptor(&data[i], (VsImage*)image, descriptor);
			m_VFR->recognize(descriptor, TOP_N_FACES, names, sim);

			delete [] descriptor;

			char result[1024];
			sprintf(result, "%s %f", names[0], sim[0]);

			if (sim[0] > SIMILARITY_THRESHOLD)
				VisageDrawing::drawMatchingIdentity(outputImage, data[i], result);
			else
				VisageDrawing::drawMatchingIdentity(outputImage, data[i], "?");
		}
	}

	else
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 3, 8);
		string error_txt = "Could not detect face!";
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width / 14, outputImage->height / 14), &outline, CV_RGB(255, 255, 255));
		cvPutText(outputImage, error_txt.c_str(), cvPoint(outputImage->width / 14, outputImage->height / 14), &font, CV_RGB(0, 0, 255));
	}

	//convert image to 4-channel image in order to draw logo
	IplImage* imageBGRA = cvCreateImage(cvSize(outputImage->width, outputImage->height), IPL_DEPTH_8U, 4);
	cvCvtColor(outputImage, imageBGRA, CV_BGR2BGRA);

	if (logo != NULL)
		VisageDrawing::drawLogo((VsImage*)imageBGRA, (VsImage*)logo);

	cvReleaseImage(&image);
	cvReleaseImage(&outputImage);

	return imageBGRA;
}

/**
*	Show and save the facial feature detection result image.
	
	@param annotatedFrame The annotated image to display and save.
	@param rootFolder Root folder; if NULL, image is not saved to file.
	@param outFolder Output folder, relative to root folder; image file is saved her; if NULL, image is not saved to file.
	@param fileName Name of the file to save the image; also used as the name of the display window.
	@param windowName Previous window name to close. If not NULL and not empty, the function will close the window by this name. If not null, the current window name will be copied into this string, so it can be closed by the next call.
*	
*/
void CFaceDetectorDoc::ShowAndSaveImage(IplImage *annotatedFrame,char *rootFolder,char *outFolder,char *fileName,char *windowName)
{
	if(!annotatedFrame) return;
	char fileName1[300];

	//destroy any previously opened window
	if(windowName) 
	{
		if(windowName[0]) cvDestroyWindow(windowName);
		strcpy(windowName,fileName);
	}

	cvNamedWindow(fileName);
	cvShowImage(fileName, annotatedFrame);
	cvWaitKey(1);

	if(rootFolder)
	{
		_chdir(rootFolder);
		_chdir(outFolder);
		sprintf(fileName1,"_D_%s",fileName);
		cvSaveImage(fileName1, annotatedFrame);
	}
}

/**
*	Performs facial features detection.
	A dialog prompts the user to choose a picture and then the algorithm tries to locate a face and its facial features in the image.
	In order for the algorithm to work, the face in the picture should have neutral expression (i.e., all facial muscles relaxed and lips closed)
	and be in frontal pose, i.e., facing the camera. To achieve the best detection please provide a good quality picture, with diffuse
	lighting conditions and where the head is in the middle of the image, taking not too much of the image area (i.e., less than 40%).

	At the end of the process the image is displayed with the FDPs drawn on top of it.

	If a face was found in the image, a FDP object will be created and saved in the current working directory.

*
*/
void CFaceDetectorDoc::OnFacialfeaturedetectionDetect()
{
	//TODO allow for more image formats
	//CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("jpg (*.jpg)|*.jpg| jpeg (*.jpeg)|*.jpeg| bmp (*.bmp)|*.bmp| ppm (*.ppm)|*.ppm| png (*.png)|*.png| All files (*.*)|*.*||"));
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
		"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
		"PNG (*.png)|*.png|"
		"GIF (*.gif)|*.gif|"
		"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
		"All files (*.*)|*.*||"));
	
	if(dlg.DoModal() == IDCANCEL)	return;
	
	char fileName[300];
	strcpy(fileName,(const char*)dlg.GetPathName());
	IplImage* annotatedFrame = DetectFeatures(fileName);
	if(annotatedFrame) 
	{
		ShowAndSaveImage(annotatedFrame,(char*)NULL,(char*)NULL,fileName,(char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}
}

/**
*	Performs face detection.
	A dialog prompts the user to choose a picture and then the algorithm tries to locate one or more faces in the image.
	
	At the end of the process the image is displayed with the facial bounding box drawn on top of it.
*
*/
void CFaceDetectorDoc::OnFacesdetectionDetect()
{
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
			"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
			"PNG (*.png)|*.png|"
			"GIF (*.gif)|*.gif|"
			"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
			"All files (*.*)|*.*||"));

	if (dlg.DoModal() == IDCANCEL)	return;

	char fileName[300];
	strcpy(fileName, (const char*)dlg.GetPathName());
	IplImage* annotatedFrame = DetectFaces(fileName);
	if (annotatedFrame)
	{
		ShowAndSaveImage(annotatedFrame, (char*)NULL, (char*)NULL, fileName, (char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}
}

/**
*	Performs age estimation.
	A dialog prompts the user to choose a picture. Then algorithm tries to locate a face and its facial features in the image. By using those
	facial features it estimates age.
	In order for the algorithm to work, the face in the picture should be in frontal pose, i.e., facing the camera. To achieve the best estimation please provide a good quality picture, with diffuse
	lighting conditions and where the head is in the middle of the image, taking not too much of the image area (i.e., less than 40%).

	At the end of the process the image is displayed with the circle around the face drawn on top of it. If estimated gender is female, the circle 
	is pink and if it is male, the circle is blue.
*
*/
void CFaceDetectorDoc::OnAgeEstimation()
{
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
		"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
		"PNG (*.png)|*.png|"
		"GIF (*.gif)|*.gif|"
		"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
		"All files (*.*)|*.*||"));
	
	if(dlg.DoModal() == IDCANCEL)	return;
	
	char fileName[300];
	strcpy(fileName,(const char*)dlg.GetPathName());
	IplImage* annotatedFrame = EstimateAge(fileName);
	if(annotatedFrame) 
	{
		ShowAndSaveImage(annotatedFrame,(char*)NULL,(char*)NULL,fileName,(char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}
}

/**
*	Performs emotion estimation.
	A dialog prompts the user to choose a picture. Then algorithm tries to locate a face and its facial features in the image. By using those
	facial features it estimates emotion.	
	At the end of the process the image is displayed with the bars that correspond the probability of each of estimated facial emotion: anger, disgust, fear, happiness, sadness and surprise.
	If no faces were detected, return NULL.
*
*/

void CFaceDetectorDoc::OnEmotionsEstimation()
{
	//TODO allow for more image formats
	//CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("jpg (*.jpg)|*.jpg| jpeg (*.jpeg)|*.jpeg| bmp (*.bmp)|*.bmp| ppm (*.ppm)|*.ppm| png (*.png)|*.png| All files (*.*)|*.*||"));
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
		"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
		"PNG (*.png)|*.png|"
		"GIF (*.gif)|*.gif|"
		"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
		"All files (*.*)|*.*||"));
	
	if(dlg.DoModal() == IDCANCEL)	return;
	
	char fileName[300];
	strcpy(fileName,(const char*)dlg.GetPathName());
	IplImage* annotatedFrame = EstimateEmotions(fileName);
	if(annotatedFrame) 
	{
		ShowAndSaveImage(annotatedFrame,(char*)NULL,(char*)NULL,fileName,(char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}
}

/**
*	Performs gender estimation.
	A dialog prompts the user to choose a picture. Then algorithm tries to locate a face and its facial features in the image. By using those
	facial features it estimates gender.
	In order for the algorithm to work, the face in the picture should be in frontal pose, i.e., facing the camera. To achieve the best estimation please provide a good quality picture, with diffuse
	lighting conditions and where the head is in the middle of the image, taking not too much of the image area (i.e., less than 40%).

	At the end of the process the image is displayed with the circle around the face drawn on top of it. If estimated gender is female, the circle 
	is pink and if it is male, the circle is blue.
*
*/
void CFaceDetectorDoc::OnGenderEstimation()
{
	//TODO allow for more image formats
	//CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("jpg (*.jpg)|*.jpg| jpeg (*.jpeg)|*.jpeg| bmp (*.bmp)|*.bmp| ppm (*.ppm)|*.ppm| png (*.png)|*.png| All files (*.*)|*.*||"));
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
		"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
		"PNG (*.png)|*.png|"
		"GIF (*.gif)|*.gif|"
		"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
		"All files (*.*)|*.*||"));
	
	if(dlg.DoModal() == IDCANCEL)	return;
	
	char fileName[300];
	strcpy(fileName,(const char*)dlg.GetPathName());
	IplImage* annotatedFrame = EstimateGender(fileName);
	if(annotatedFrame) 
	{
		ShowAndSaveImage(annotatedFrame,(char*)NULL,(char*)NULL,fileName,(char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}
}

/**
*	Performs face recognition.
	A dialog prompts the user to choose a picture. Then algorithm measures similarity between human faces and recognizes a person's identity from the image. 
	In order for the algorithm to work, the face in the picture should be in frontal pose, i.e., facing the camera. To achieve the best estimation please provide a good quality picture, with diffuse
	lighting conditions and where the head is in the middle of the image, taking not too much of the image area (i.e., less than 40%).

	At the end of the process the image is displayed with the name of the recognized person or the name of the person with the most similar face found in the VisageFaceRecognition gallery.
*
*/
void CFaceDetectorDoc::OnFaceRecognition()
{
	//TODO allow for more image formats
	//CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("jpg (*.jpg)|*.jpg| jpeg (*.jpeg)|*.jpeg| bmp (*.bmp)|*.bmp| ppm (*.ppm)|*.ppm| png (*.png)|*.png| All files (*.*)|*.*||"));
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Image Files (*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.ppm)|*.jpg;*.jpeg;*.bmp;*.ppm;*.png;*.gif|"
			"JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
			"PNG (*.png)|*.png|"
			"GIF (*.gif)|*.gif|"
			"Bitmap files (*.bmp;*.dib)|*.bmp;*.dib|"
			"All files (*.*)|*.*||"));

	//set recognition dialog initial directory to last used directory (or default)
	if (m_lastRecognitionPath == "")
	{
		char workingDir[MAX_PATH]; 
		getcwd(workingDir, MAX_PATH);
		m_lastRecognitionPath = string(workingDir).substr(0, string(workingDir).find_last_of("\\/")) + "\\OpenGL\\data\\FaceDetector\\Famous Actors";
	}
	dlg.m_ofn.lpstrInitialDir = (TCHAR*)m_lastRecognitionPath.c_str();

	if (dlg.DoModal() == IDCANCEL)	return;

	char fileName[300];
	strcpy(fileName, (const char*)dlg.GetPathName());
	IplImage* annotatedFrame = RecognizeFace(fileName);
	if (annotatedFrame)
	{
		ShowAndSaveImage(annotatedFrame, (char*)NULL, (char*)NULL, fileName, (char*)NULL);
		cvReleaseImage(&annotatedFrame);
	}

	//remember last used directory for recognition dialog
	m_lastRecognitionPath = string(fileName).substr(0, string(fileName).find_last_of("\\/"));
}

/**
*	Performs batch facial features detection.
*	A dialog prompts the user to choose a batch processing file.
*
*/
void CFaceDetectorDoc::OnBatchDetection()
{
	char inString[200];
	char fileName[200];
	char fileName2[200];
	char windowName[200];
	char rootFolder[300];
	char inFolder[300];
	char outFolder[300];
	char basePath[300];
	FDP* f = new FDP();

	if(!initFaceAnalyser())
		return;

	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Text Files (*.txt)|*.txt|All files (*.*)|*.*||"));
	
	if(dlg.DoModal() == IDCANCEL)	return;
	FILE *listFile = fopen((const char*)dlg.GetPathName(),"r");
	if (!listFile)
		return;
	windowName[0]=0;

	strcpy(basePath,dlg.GetFolderPath());
	_getcwd(rootFolder,300);
	strcpy(inFolder,basePath);
	strcpy(outFolder,basePath);
	while(fgets(inString,200,listFile))
	{
		if(inString[strlen(inString)-1]==10) inString[strlen(inString)-1]=0; //clip Line Feed character from the end
		if(strlen(inString)==0) continue; //skip empty lines
		if(inString[0]=='#') continue; //skip comments

		if(inString[0]=='$') //process special commands
		{
			//input folder
			if(strstr(inString,"$INFOLDER:")) sprintf(inFolder,"%s\\%s",basePath,inString+10);
			//output folder
			if(strstr(inString,"$OUTFOLDER:")) sprintf(outFolder,"%s\\%s",basePath,inString+11);
			//process all files in folder
			if(strstr(inString,"$PROCESS_ALL_FILES"))
			{
				char tmpstr[100];
				struct _finddata_t c_file;
				intptr_t hFile;

				sprintf(tmpstr,"*.jpg");
				_chdir(rootFolder);
				_chdir(inFolder);
				/* Find first .jpg file in the given directory */
				if( (hFile = _findfirst( tmpstr, &c_file )) != -1L )
				{
					do
					{
						strcpy(fileName,c_file.name);
						sprintf(fileName2, "%s\\%s", inFolder, fileName); //create full path
						_chdir(rootFolder);	//detection must be done in root path

						//strcpy(outFolder, basePath);
						IplImage* annotatedFrame = DetectEverything(fileName2);
						if(annotatedFrame) 
						{
							ShowAndSaveImage(annotatedFrame,rootFolder,outFolder,fileName,windowName);
							cvReleaseImage(&annotatedFrame);
						}
					}
					while(_findnext( hFile, &c_file ) == 0);
				}
				_findclose( hFile );
			}
		}
		else //process a single file
		{
			strcpy(fileName,inString);
			sprintf(fileName2, "%s\\%s", inFolder, fileName);
			_chdir(rootFolder);
			//_chdir(inFolder);
			IplImage* annotatedFrame = DetectEverything(fileName2);

			if(annotatedFrame) 
			{
				ShowAndSaveImage(annotatedFrame,rootFolder,outFolder,fileName,windowName);
				cvReleaseImage(&annotatedFrame);
			}
		}
		_chdir(rootFolder);
	}
	if(windowName[0]) cvDestroyWindow(windowName);
}

/**
* Loads and adjusts all images in given folder and add descriptors to VisageFaceRecognition gallery for each one.
*
*/
int CFaceDetectorDoc::createGallery(const char* folderPath)
{
	if (!m_Detector)
		return 0;

	struct _finddata_t c_file;
	intptr_t hFile;
	char tmpstr[500];
	char filename[260];

	FaceData data;

	sprintf(tmpstr, "%.450s\\*.jpg", folderPath);

	// Find first .jpg file in the given directory
	if ((hFile = _findfirst(tmpstr, &c_file)) == -1L)
	{
		return 0; //no files found 
	}

	do
	{
		strcpy(filename, c_file.name);
		
		char filePath[100]; 

		strcpy(filePath, folderPath);
		strcat(filePath, "/");
		strcat(filePath, filename);
		
		// load and adjust image and add descriptor
		VsImage* img = (VsImage*)loadAndAdjustImage(filePath);
		char *nameTag = strtok(filename, ".");

		int n_faces = this->m_Detector->detectFacialFeatures(img, &data, 1);

		if (n_faces > 0)
			m_VFR->addDescriptor(img, &data, nameTag);

		if (_findnext(hFile, &c_file) != 0)
			break;

	} while (1);

	return 1;

	_findclose(hFile);

}