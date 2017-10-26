///////////////////////////////////////////////////////////////////////////////
// 
// (c) Visage Technologies AB 2002 - 2016  All rights reserved. 
// 
// This file is part of visage|SDK(tm). 
// Unauthorized copying of this file, via any medium is strictly prohibited. 
// 
// No warranty, explicit or implicit, provided. 
// 
// This is proprietary software. No part of this software may be used or 
// reproduced in any form or by any means otherwise than in accordance with
// any written license granted by Visage Technologies AB. 
// 
/////////////////////////////////////////////////////////////////////////////


#ifndef __VisageFeaturesDetector_h__
#define __VisageFeaturesDetector_h__

#ifdef VISAGE_STATIC
	#define VISAGE_DECLSPEC
#else

	#ifdef VISAGE_EXPORTS
		#define VISAGE_DECLSPEC __declspec(dllexport)
	#else
		#define VISAGE_DECLSPEC __declspec(dllimport)
	#endif

#endif

#include "FaceData.h"

using namespace std;

namespace VisageSDK
{

class VisageDetector;
class Candide3Model;

/** Faces and facial features detector implementation.
* 
* This class can be used to detect one or more faces and their facial features in an image. The input is an image bitmap or an image file in one of the supported file formats: JPEG, PNG, BMP or PPM.
*
* Function @ref detectFaces() performs a fast face detection on the image. The result is, for each detected face, a VsRect object containing facial bounding box.
*
* Function @ref detectFacialFeatures() performs a face and facial features detection on the image. The results is, for each detected face, FaceData object containing 3D head pose, coordinates of facial feature points 
* (e.g. pupils, nose tip, lip corners etc.), 3D face model fitted to the face and more. Please refer to the FaceData documentation for detailed description of returned data.
*
* To use the detector, it must first be initialized by calling the function @ref Initialize().
*
* Implemented in libVisageVision.lib
*
* \if IOS_DOXY
* Demonstrated in <a href="../facedetect.html">FaceDetector</a> sample project.
* \elseif MACOSX_DOXY
* Demonstrated in <a href="../facedetect.html">FaceDetector</a> sample project.
* \elseif LINUX_DOXY
* Demonstrated in <a href="../../../Samples/Linux/build/msvc100/VisageDetectorDemo/doc/index.html">FaceDetector</a> sample project.
* \elseif REDHAT_DOXY
* Demonstrated in <a href="../../../Samples/Linux/build/msvc100/VisageDetectorDemo/doc/index.html">FaceDetector</a> sample project.
* \else
* Demonstrated in <a href="../../../Samples/OpenGL/build/msvc140/FaceDetector/doc/index.html">FaceDetector</a> sample project.
* \endif
*
*/
class VISAGE_DECLSPEC VisageFeaturesDetector {

public:

	/** Constructor.
	*	
	*/
	VisageFeaturesDetector();

	/** Destructor.
	*/
	~VisageFeaturesDetector();

	/** Initialise the feature detector. 
	*
	* Several data files are needed for facial features detection and the path to the data folder containing these data files must be passed to the initialization function, for example:
	*	
	* \code
	* std::string dataPath("."); //Assuming the current working folder contains data files(from "bdtsdata" folder).	
	*
	* m_Detector->Initialize(dataPath.c_str());
	* \endcode
	*
	* The data folder must contain:
	* - a subfolder "bdtsdata" with complete contents as provided in visage|SDK;
	* - files candide3.fdp, candide3.wfm, jk_300.fdp, jk_300.wfm and FaceDetector.cfg.
	*
	* In visage|SDK, the data folder with necessary files is Samples/data. These files must be distributed with any application using VisageFeaturesDetector.
	* 
	* @param path the path to the detector data files
	* @return true if successful
	*/
	bool Initialize(const char* path);

	/**
	* Performs face detection in a still image.
	*
	* The algorithm detects one or more faces. For each detected face a square facial bounding box is returned.
	*
	* The results are returned in form of VsRect objects. An array of VsRect objects passed to this method as output parameter should be allocated to maxFaces size.
	* For example:
	*
	* \code
	* VsRect* faces = new VsRect[maxFaces];
	*
	* n = this->m_Detector->detectFaces(image, faces, maxFaces);
	* \endcode
	*
	* After this call, n contains the number of detected faces. The first n members of the faces array are filled with resulting bounding boxes for each detected face.
	* If maxFaces is smaller than the number of faces actually detected in the image, the function will return only first maxFaces detected faces.
	*
	* VsImage is the image storage class similar to IplImage from OpenCV, it has the same structure and members so it can be used like IplImage. Please refer to
	* OpenCV documentation for details of accessing IplImage data members; the basic members are the size of the image (frame->width, frame->height) and the pointer to the actual pixel data of the image (frame->imageData).
	*
	* Following image formats are supported:
	* - VISAGE_FRAMEGRABBER_FMT_RGB: each pixel of the image is represented by three bytes representing red, green and blue channels, respectively.
	* - VISAGE_FRAMEGRABBER_FMT_RGBA: each pixel of the image is represented by four bytes representing red, green, blue and alpha (ignored) channels, respectively.
	* - VISAGE_FRAMEGRABBER_FMT_LUMINANCE: each pixel of the image is represented by one byte representing the luminance (gray level) of the image.
	* Origin must be:
	* - VISAGE_FRAMEGRABBER_ORIGIN_TL: Origin is the top left pixel of the image. Pixels are ordered row-by-row starting from top left.
	*
	* Note that the input image is internally converted to grayscale.
	*
	* @param frame the input image.
	* @param faces pointer to an array of VsRect objects in which the results will be returned.
	* @param maxFaces maximum number of faces to be detected
	* @param minFaceScale scale of smallest face to be searched for, defined as decimal fraction [0.0 - 1.0] of input image size (min(width, height))
	* @param maxFaceScale scale of largest face to be searched for, defined as decimal fraction [0.0 - 1.0] of input image size (min(width, height))
	* @param useRefinementStep if set to true, additional refinement algorithm will be used resulting with more precise facial bounding boxes and lower FPR, but higher detection time 
	* @return number of detected faces (0 or more)
	*/
	int detectFaces(VsImage* frame, VsRect* faces, int maxFaces = 1, float minFaceScale = 0.1f, float maxFaceScale = 1.0f, bool useRefinementStep = true);

	/**
	* Performs faces and facial features detection in a still image.
	* 
	* The algorithm detects one or more faces and their features. The results are, for each detected face, the 3D head pose, gaze direction, eye closure, the coordinates of facial feature points 
	* (e.g. chin tip, nose tip, lip corners etc.) and 3D face model fitted to the face. If outputOnly2DFeatures is set, only the 2D feature points will be returned.
	*
	* The results are returned in form of FaceData objects. An array of FaceData objects passed to this method as output parameter should be allocated to maxFaces size. 
	* For example:
	* 
	* \code
	* FaceData* data = new FaceData[maxFaces];
	*
	* n = this->m_Detector->detectFacialFeatures(image, data, maxFaces, minFaceScale);
	* \endcode
	*
	* After this call, n contains the number of faces actually detected. The first n members of the data array are filled with resulting data for each detected face.
	* Please refer to the FaceData documentation for detailed description of returned parameters. If maxFaces is smaller than the number of faces actually present in the image, the function will return only first maxFaces detected faces.
	*
	* VsImage is the image storage class similar to IplImage from OpenCV, it has the same structure and members so it can be used like IplImage. Please refer to 
	* OpenCV documentation for details of accessing IplImage data members; the basic members are the size of the image (frame->width, frame->height) and the pointer to the actual pixel data of the image (frame->imageData).
	*
	* Following image formats are supported:
	* - VISAGE_FRAMEGRABBER_FMT_RGB: each pixel of the image is represented by three bytes representing red, green and blue channels, respectively.
	* - VISAGE_FRAMEGRABBER_FMT_RGBA: each pixel of the image is represented by four bytes representing red, green, blue and alpha (ignored) channels, respectively.
	* - VISAGE_FRAMEGRABBER_FMT_LUMINANCE: each pixel of the image is represented by one byte representing the luminance (gray level) of the image.
	* Origin must be:
	* - VISAGE_FRAMEGRABBER_ORIGIN_TL: Origin is the top left pixel of the image. Pixels are ordered row-by-row starting from top left.
	*
	* Note that the input image is internally converted to grayscale.
	*
	* @param frame the input image. 
	* @param output pointer to an array of FaceData objects in which the results will be returned.
	* @param maxFaces maximum number of faces to be detected
	* @param minFaceScale scale of smallest face to be searched for, defined as decimal fraction [0.0 - 1.0] of input image size (min(width, height))
	* @param maxFaceScale scale of largest face to be searched for, defined as decimal fraction [0.0 - 1.0] of input image size (min(width, height))
	* @param outputOnly2DFeatures if set, detection time will be reduced and only FeaturePoints2D will be returned
	* @return number of detected faces (0 or more)
	*
	* @see FaceData
	*/
	int detectFacialFeatures(VsImage* frame, FaceData* output, int maxFaces = 1, float minFaceScale = 0.1f, float maxFaceScale = 1.0f, bool outputOnly2DFeatures = false);

	bool fitModelToFaceD(FDP *input, FaceData *output, VsImage *frame, float pose[6], bool freezeRot = false, bool freezePos = false);

	bool fitModelToFace(FDP* input, FaceData* output, VsImage * frame, float initYaw);

	Candide3Model* m_model;
private:

	VisageFeaturesDetector(const char *dataPath);

	void calculateFDP(FDP* f, int w, int h, VsMat* vert, bool _3D);
	void setFDPIndices(FDP* f);

	bool readConfiguration(const char* datapath);
	bool loadModel(const string &settings, const char* cfgDirPat, Candide3Model **model_, int modelType);

	VisageDetector* m_detector;
	Candide3Model* m_au_model;
	Candide3Model* m_mesh_model;
	bool use_pose_model;
	bool use_au_model;
	bool use_mesh_model;

	char* m_cfg;

	bool m_initialised;

	char m_detectCfgDirPath[300];
};

}
#endif // __VisageFeaturesDetector_h__

