// FaceDetectorDoc.h : interface of the CFaceDetectorDoc class
//


#pragma once

#include "visageVision.h"
#include "FolderManager.h"
#include "VisageDrawing.h"
#include "VisageFaceRecognition.h"

#include "cv.h"

using namespace VisageSDK;


/*! \mainpage  visage|SDK FaceDetector example project
*
* \if WIN64_DOXY
* \htmlonly
* <table border="0">
* <tr>
* <td width="32"><a href="../../../../../../bin64/DEMO_FaceDetector_x64.exe"><img src="../../../../../../doc/OpenGL/images/run_sample.png" border=0 title="Run Sample (this may not work in all browsers, in such case please run DEMO_FaceDetector_x64.exe from the visageSDK\bin64 folder.)"></a></td>
* <td width="32"><a href="../../../../../../bin64"><img src="../../../../../../doc/OpenGL/images/open_bin_folder.png" border=0 title="Open Binary Folder (visageSDK\bin)"></a></td>
* <td width="32"><a href="../../../../data/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_data_folder.png" border=0 title="Open Data Folder"></a></td>
* <td width="32"><a href="../../../../build/msvc140/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_project_folder.png" border=0 title="Open Project Folder"></a></td>
* <td width="32"><a href="../../../../source/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_source_folder.png" border=0 title="Open Source Code Folder"></a></td>
* </tr>
* </table>
* \endhtmlonly
* \else
* \htmlonly
* <table border="0">
* <tr>
* <td width="32"><a href="../../../../../../bin/DEMO_FaceDetector.exe"><img src="../../../../../../doc/OpenGL/images/run_sample.png" border=0 title="Run Sample (this may not work in all browsers, in such case please run DEMO_FaceDetector.exe from the visageSDK\bin folder.)"></a></td>
* <td width="32"><a href="../../../../../../bin"><img src="../../../../../../doc/OpenGL/images/open_bin_folder.png" border=0 title="Open Binary Folder (visageSDK\bin)"></a></td>
* <td width="32"><a href="../../../../data/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_data_folder.png" border=0 title="Open Data Folder"></a></td>
* <td width="32"><a href="../../../../build/msvc140/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_project_folder.png" border=0 title="Open Project Folder"></a></td>
* <td width="32"><a href="../../../../source/FaceDetector"><img src="../../../../../../doc/OpenGL/images/open_source_folder.png" border=0 title="Open Source Code Folder"></a></td>
* </tr>
* </table>
* \endhtmlonly
* \endif
*
* The FaceDetector example project demonstrates the following capabilities of visage|SDK:
*
* - facial features detection in still images
* - face analysis in still images: age, gender and emotions estimation
* - face recognition in still images
*
* To use the application: 
*
* Facial Feature Detection -> Detect Facial Features in Image will prompt the choice of an image and perform facial features detection. Several test images are available in the folder <a href="../../../../data/FaceDetector">Samples/OpenGL/data/FaceDetector</a>. \n
*(Note: Without the valid license, results will be shown with a 2 seconds delay)
*
* Facial Feature Detection -> Detect Faces in Image will prompt the choice of an image and perform face detection. Several test images are available in the folder <a href="../../../../data/FaceDetector">Samples/OpenGL/data/FaceDetector</a>. \n
*(Note: Without the valid license, results will be shown with a 2 seconds delay)
*
* Face Analyser -> Estimate Age in Image will prompt the choice of an image and perform age estimation. The result is displayed in the middle of the detected face as a green text. \n
*(Note: Without the valid license, results will be shown with a 4 seconds delay)
*
* Face Analyser -> Estimate Gender in Image will prompt the choice of an image and perform gender estimation. The result will be displayed on the screen: if estimated gender is female, pink circle will be drawn around the face and if it is male, the circle will be blue. \n
*(Note: Without the valid license, results will be shown with a 4 seconds delay)
*
* Face Analyser -> Estimate Emotion in Image will prompt the choice of an image and perform emotion estimation. It returns the image with the bars that correspond to the probability of each of estimated facial emotions: anger, disgust, fear, happiness, sadness, surprise and neutral. \n
*(Note: Without the valid license, results will be shown with a 4 seconds delay)
*
* Face Recognition -> Match Faces in Images will prompt the choice of an image and perform face recognition. Calls helper method initFaceRecognition() which initializes new VisageFaceRecognition object and creates gallery from the images located in Famous Actors/gallery/ folder. It returns the image with the name of the recognized person or the name of the person with the most similar face found in the VisageFaceRecognition gallery. \n
*(Note: Without the valid license, results will be shown with a 7 seconds delay)
*
* Batch Processing -> Batch Detection and Analysis will prompt the choice of a text file with a list of images to process (each line should contain one file name, no path, images should be in the same folder as the list file).
* Facial feature detection altogether with emotions and gender estimation will be performed on each image from the list. The results will be displayed on the screen and saved in an image file with the same name as the original image but prefixed with "_D_".
* The folder <a href="../../../../data/FaceDetector">Samples/OpenGL/data/FaceDetector</a> contains a sample file list.txt with the list of all provided sample images. \n
*(Note: Without the valid license, results will be shown with a 8 seconds delay)
*
* The batch detection also supports more powerful syntax allowing to process folders with images. A commented example of such syntax is:
*
* \# Specify input folder, relative to the location of this file\n
* $INFOLDER:myInputFolder
*
* \# Specify output folder, relative to the location of this file\n
* $OUTFOLDER:myOutputFolder
*
* \# Process all files from input folder, put result files in output folder\n
* $PROCESS_ALL_FILES
* 
* This simple syntax will process all image files in "myInputFolder" and place the results in "myOutputFolder". Folder paths are relative to the location of the batch file itself. These statements can be repeated in order to process multiple folders. Output folders must exist (application will not create them).
*
* The specific classes and methods that demonstrate visage|SDK are:
* - CFaceDetectorDoc::CFaceDetectorDoc(): Initialises the Facial Feature Detector @ref CFaceDetectorDoc::m_Detector.
* - CFaceDetectorDoc::OnFacialfeaturedetectionDetect(): Performs facial features detection on a still image and displays the result
* - CFaceDetectorDoc::OnFacesdetectionDetect(): Performs face detection on a still image and displays the result as a face bounding box in image
* - CFaceDetectorDoc::OnEmotionsEstimation(): Performs emotion estimation. 	
* - CFaceDetectorDoc::OnGenderEstimation(): Performs gender estimation.
* - CFaceDetectorDoc::OnAgeEstimation(): Performs age estimation.
* - CFaceDetectorDoc::OnFaceRecognition(): Performs face recognition.
* - CFaceDetectorDoc::OnBatchDetection(): Performs batch processing of images as described above.
*
*/

/**
* The document.
* Main application document.
*/
class CFaceDetectorDoc : public CDocument
{
protected: // create from serialization only
	CFaceDetectorDoc();
	DECLARE_DYNCREATE(CFaceDetectorDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CFaceDetectorDoc();

	bool CFaceDetectorDoc::initFaceAnalyser();
	bool CFaceDetectorDoc::initFaceRecognition();

	IplImage* CFaceDetectorDoc::DetectFeatures(char *fileName);

	IplImage* CFaceDetectorDoc::DetectFaces(char *fileName);

	IplImage* CFaceDetectorDoc::EstimateAge(char *fileName);

	IplImage* CFaceDetectorDoc::EstimateEmotions(char *fileName);

	IplImage* CFaceDetectorDoc::EstimateGender(char *fileName);

	IplImage* CFaceDetectorDoc::RecognizeFace(char *filename);
	
	IplImage* CFaceDetectorDoc::DetectEverything(char *fileName);

	void CFaceDetectorDoc::ShowAndSaveImage(IplImage *annotatedFrame,char *rootFolder,char *outFolder,char *fileName,char *windowName);

	int CFaceDetectorDoc::createGallery(const char* folderPath);

	/**
	* This flag indicates that person generic appearance model has been loaded and the system is thus ready for feature detection purposes.
	*/
	//bool genericAamLoaded;


	/**
	* The Facial Features Detector object (VisageFeaturesDetector).
	* It is used to locate facial definition points on a face.
	*/
	VisageFeaturesDetector* m_Detector;

	VisageFaceAnalyser* m_VFA;

	VisageFaceRecognition* m_VFR;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFacialfeaturedetectionDetect();
	afx_msg void OnFacesdetectionDetect();
	afx_msg void OnBatchDetection();
	afx_msg void OnAgeEstimation();
	afx_msg void OnEmotionsEstimation();
	afx_msg void OnGenderEstimation();	
	afx_msg void OnFaceRecognition();
};


