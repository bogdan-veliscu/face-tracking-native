// VisionExampleDoc.cpp : implementation of the CVisionExampleDoc class
//

#include "stdafx.h"
#include "VisionExample.h"
#include "VisionExampleDoc.h"
#include "OpenGLWndFaceTracker.h"
#include "FaceData.h"
#include "MainFrm.h"
#include <process.h> 
#include "Common.h"
#include "VisageRendering.h"

#include "StrStreamUtils.h"
#include <highgui.h>
#include <videoInput.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define VS_WRITE_AU
#define VS_WRITE_HEADPOSE

static int startFrame = -1;
static int endFrame = -1;
static int slowdownFrame = -1;
static int slowdownTime = -1;

//pure track time variables
long startTime = 0;
long endTime = 0;
long elapsedTime = 0;

//logo image
IplImage* logo = NULL;
int logoViewportWidth;
int logoViewportHeight;

std::string previousImageFolder = ".";
std::string previousVideoFolder = ".";
std::string previousBatchFolder = ".";

/////////////////////////////////////////////////////////////////////////////
// CVisionExampleDoc

IMPLEMENT_DYNCREATE(CVisionExampleDoc, CDocument)

BEGIN_MESSAGE_MAP(CVisionExampleDoc, CDocument)
	//{{AFX_MSG_MAP(CVisionExampleDoc)


	//}}AFX_MSG_MAP
	ON_COMMAND(ID_TRACKING_STOP, &CVisionExampleDoc::OnTrackingStop)
	ON_COMMAND(ID_TRACKING_TRACKFROMAVIVIDEO, &CVisionExampleDoc::OnTrackingTrackfromavivideo)
	ON_COMMAND(ID_TRACKING_TRACKFROMCAM, &CVisionExampleDoc::OnTrackingTrackfromcam)
	ON_COMMAND(ID_TRACKING_CAMERASETTINGS, &CVisionExampleDoc::OnTrackingCamerasettings)
	ON_COMMAND(ID_TRACKING_BATCHPROCESSING, &CVisionExampleDoc::OnTrackingBatchprocessing)
	ON_COMMAND(ID_TRACKING_TRACKINIMAGE, &CVisionExampleDoc::OnTrackingTrackinimage)
	ON_COMMAND(ID_TRACKING_WRITEDATA, &CVisionExampleDoc::OnTrackingWritedata)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVisionExampleDoc construction/destruction

CVisionExampleDoc::CVisionExampleDoc()
{
	m_Tracker = 0;
	m_OpenGLWnd = 0;
	m_MainWnd = 0;
	trackerUsed = false;
	testImage = 0;
	datafile = 0;
	headerSet = false;
	writeDataEnabled = false;
	trackingThreadHandle = NULL;
	cam_device = 0;
	VI = 0;

	//Common::RedirectIOToConsole();
}

void CVisionExampleDoc::init()
{
	if(!m_OpenGLWnd || !m_MainWnd)
	{
		// get the OpenGLWindow
		m_MainWnd = (CMainFrame*)AfxGetMainWnd();
		m_OpenGLWnd = m_MainWnd->GetOpenGLWnd();
	}
	char defaultConfigFile[200];
	FILE *f = fopen("DefaultConfiguration.txt","r");
	fgets(defaultConfigFile,200,f);
	fclose(f);
	defaultConfigFile[strlen(defaultConfigFile)-1] = 0; //remove line feed character
	if(!m_Tracker)
	{
		m_Tracker = new VisageTracker(defaultConfigFile); 
	}
	char path[300];
	_getcwd(path,300);
	m_MainWnd->ShowAvailConfigFiles(path,defaultConfigFile);
	m_MainWnd->ShowAvailCamResolutions();
	m_MainWnd->ShowAvailCamDevices();
	m_MainWnd->ShowAvailMaxFaces();

	//load logo image, working directory is Samples/data
	logo = cvLoadImage("../OpenGL/data/logo.png", CV_LOAD_IMAGE_UNCHANGED);
}

CVisionExampleDoc::~CVisionExampleDoc()
{
	if (m_Tracker)
		delete m_Tracker;

	if (logo)
		cvReleaseImage(&logo);
}

void CVisionExampleDoc::OnCloseDocument()
{
	if (m_Tracker) {
		m_OpenGLWnd->trackerActive = false;
	
		WaitForSingleObject(trackingThreadHandle, INFINITE);
		
		delete m_Tracker;
		m_Tracker = 0;
	}

	CDocument::OnCloseDocument();
}


/////////////////////////////////////////////////////////////////////////////
// CVisionExampleDoc serialization

void CVisionExampleDoc::Serialize(CArchive& ar)
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

/////////////////////////////////////////////////////////////////////////////
// CVisionExampleDoc diagnostics

#ifdef _DEBUG
void CVisionExampleDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVisionExampleDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVisionExampleDoc commands

void writeDataHeader(FILE *fp, int auNum) {

	char header[2000] = {0};
	char tmp[50];

	strcat(header, "Frame;Tracking status;");
#ifdef VS_WRITE_HEADPOSE
	strcat(header, "Head x;Head y;Head z;Head pitch;Head yaw;Head roll;");
#endif
#ifdef VS_WRITE_AU
	for (int i=0; i<auNum;i++) {
		sprintf(tmp, "AU%d;", i);
		strcat(header, tmp);
	}
#endif

	header[strlen(header)-1] = 0; // earse last ;
	fprintf(fp, "%s\n", header);
}

void CVisionExampleDoc::finishWriteResults()
{
	if (datafile) {
		fclose(datafile);
		datafile = 0;
		headerSet = false;
	}
}

/**
* Stop tracking.
* It is called when Tracking->Stop menu item is chosen.
*
* This function stops the face tracking operation. 
*
*/

void CVisionExampleDoc::OnTrackingStop() 
{
	if(m_Tracker)
	{
		m_OpenGLWnd->trackerActive = false;
	
		WaitForSingleObject(trackingThreadHandle, INFINITE);
	}

	if(m_OpenGLWnd)
	{
		m_OpenGLWnd->RenderToScreen();
	}

	finishWriteResults();
}

void CVisionExampleDoc::writeResults(int track_stat, FaceData &trackingData)
{
	if (!datafile)
		return;

	//write data to file
	std::string data = "";
	char tmp[100];

	if(!headerSet) {
		writeDataHeader(datafile, trackingData.actionUnitCount);
		headerSet = true;
		frameNum = 1; // first frame is used for initialization, thus skipped
	}

	// frame index and tracking status
	sprintf(tmp, "%d;", frameNum++);
	data.append(tmp);
	switch (track_stat) {
		case TRACK_STAT_OK:
			data.append("OK;");
			break;
		case TRACK_STAT_INIT:
			data.append("INIT;");
			break;
		case TRACK_STAT_RECOVERING:
			data.append("RECOVERING;");
			break;
		default:
			data.append("OFF;");
			break;
	}

#ifdef VS_WRITE_HEADPOSE
	//Head position and rotation
	float hr[3] = {0.0f,0.0f,0.0f};
	float ht[3] = {0.0f,0.0f,0.0f};
	if (track_stat == TRACK_STAT_OK) {
		for(int i=0;i<3;i++) {
			hr[i] = (180.0f * trackingData.faceRotation[i]) / 3.14f;
			ht[i] = trackingData.faceTranslation[i];
		}
	}
	sprintf(tmp, "%f;%f;%f;%f;%f;%f;", ht[0],ht[1],ht[2],hr[0],hr[1],hr[2]);
	data.append(tmp);
#endif
#ifdef VS_WRITE_AU
	// AUs
	for (int i=0; i < trackingData.actionUnitCount; i++) {
		sprintf(tmp, "%f;", (track_stat == TRACK_STAT_OK) ? trackingData.actionUnits[i] : 0.0f);
		data.append(tmp);
	}
#endif

	data.erase(data.end()-1);
	
	fprintf(datafile, "%s\n", data.c_str());
}

static void displayResults(CVisionExampleDoc *td, int* track_stat, FaceData* trackingData, VsImage* frame, VsImage* logo, int n = 1)
{
	// display the frame rate, position and other info
	if(td->m_MainWnd)
	{
		char tmpmsg[400];
		float trackingFrameRate = trackingData[0].frameRate;
		
		float r[3];
		float t[3];
		if(track_stat[0] == TRACK_STAT_OK)
		{
				
			for(int i=0;i<3;i++) {
				r[i] = (180.0f * trackingData[0].faceRotation[i]) / 3.141592f;
				t[i] = trackingData[0].faceTranslation[i] * 100.0f; //translation is expressed in meters so this gives approximate values in centimeters if camera focus distance parameter f in fbft is set correctly
			}
		}
		else
		{
			for(int i=0;i<3;i++) {
				r[i] = 0.0f;
				t[i] = 0.0f; 
			}
		}

		const char* status = "";

		switch (track_stat[0]) {
			case TRACK_STAT_OK:			status = "OK";			break;
			case TRACK_STAT_INIT:		status = "INIT";		break;
			case TRACK_STAT_RECOVERING:	status = "RECOVERING";	break;
			default:					status = "OFF";			break;
		}
			
		sprintf(tmpmsg,"%4.1f ms (track %ld ms) | %4.1f FPS | Head position %+5.1f %+5.1f %+5.1f | Rotation (deg) %+5.1f %+5.1f %+5.1f | %dx%d | %s",
			1000.0f/trackingFrameRate, elapsedTime, trackingFrameRate, t[0], t[1], t[2], r[0], r[1], r[2], frame->width, frame->height, status);
		CString* message = new CString(tmpmsg); //this is deleted at the receiving side so there is no memory leak
		::SendNotifyMessage(td->m_MainWnd->m_hWnd, WM_MYMESSAGE, (WPARAM) message, 0);

		float aspect = (float)frame->width / (float)frame->height;

		int winWidth = floor(td->m_OpenGLWnd->mWindowHeight * aspect);
		int winHeight = td->m_OpenGLWnd->mWindowHeight;

		td->m_OpenGLWnd->setOpenGLContext();

		VisageRendering::DisplayResults(&trackingData[0], track_stat[0], winWidth, winHeight, frame, DISPLAY_DEFAULT);
		for (int i = 1; i < n; i++)
			VisageRendering::DisplayResults(&trackingData[i], track_stat[i], winWidth, winHeight, frame,
				DISPLAY_DEFAULT ^ DISPLAY_FRAME ^ DISPLAY_TRACKING_QUALITY );

		//display logo
		if (logo != NULL)
			VisageRendering::DisplayLogo(logo, winWidth, winHeight);

		td->m_OpenGLWnd->swapOpenGLBuffers();
	}

	if (td->m_OpenGLWnd) td->m_OpenGLWnd->RenderToScreen();
}

static void displayResults(CVisionExampleDoc *td, int track_stat, FaceData &trackingData, VsImage* frame, VsImage* logo)
{
	displayResults(td, &track_stat, &trackingData, frame, logo, 1);
}

static long getCurrentTimeMs(bool init = false)
{
	static long initialTime = 0;

	struct _timeb timebuffer;
	_ftime( &timebuffer );

	long clockTime = 1000 * (long)timebuffer.time + timebuffer.millitm;

	if(init)
		initialTime = clockTime;
	
	clockTime -= initialTime;
	
	return clockTime;
}

void CVisionExampleDoc::trackFromVideo(void *thisdoc)
{
	CVisionExampleDoc *td = (CVisionExampleDoc *)thisdoc;

	int format = VISAGE_FRAMEGRABBER_FMT_BGR;

	bool video_file_sync = true;

	printf("Tracking from video %s...\n", td->videoFileName.c_str());

	CvCapture* capture = cvCaptureFromFile( td->videoFileName.c_str() );

	getCurrentTimeMs(true);

	float frameTime = 1000.0/(float)cvGetCaptureProperty( capture, VS_CAP_PROP_FPS );
	int frameCount = 0;

	int max_faces = td->m_MainWnd->GetMaxFaces();

	FaceData trackingData[10];

	int *track_stat = 0;

	while (td->m_OpenGLWnd->trackerActive && cvGrabFrame(capture)) {

		long currentTime = getCurrentTimeMs();

		while(video_file_sync && currentTime > frameTime*(frameCount+1) ) // skip frames if track() is too slow
		{
			if(!td->m_OpenGLWnd->trackerActive || !cvGrabFrame(capture))
				break;

			frameCount++;
			currentTime = getCurrentTimeMs();
		}

		while(video_file_sync && currentTime < frameTime*(frameCount-5)) // wait if track() is too fast
		{
			Sleep(1);
			currentTime = getCurrentTimeMs();

			if(!td->m_OpenGLWnd->trackerActive)
				break;
		}

		const IplImage *framePtr = cvRetrieveFrame(capture); // grab latest frame

		if (!framePtr) break;

		frameCount++;

		startTime = getCurrentTimeMs();
		track_stat = td->m_Tracker->track(framePtr->width,framePtr->height,framePtr->imageData, trackingData, format, framePtr->origin, framePtr->widthStep, frameTime*frameCount);
		endTime = getCurrentTimeMs();
		elapsedTime = endTime - startTime;

		displayResults(td, track_stat, trackingData, (VsImage*)framePtr, (VsImage*)logo, max_faces);

		if (td->writeDataEnabled) td->writeResults(track_stat[0], trackingData[0]);
	}

	td->m_Tracker->stop();
	td->finishWriteResults();

	cvReleaseCapture(&capture);

	printf("Stop tracking from video.\n");

	_endthread();
}


/**
* The main function for tracking from a video file. 
* It is called when the Tracking->Track From Avi Video menu item is chosen.
*
* @see OnTrackingTrackfromcam()
*/
void CVisionExampleDoc::OnTrackingTrackfromavivideo()
{
	m_OpenGLWnd->trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	finishWriteResults();

	// get the current path
	visageFolderManager.pushPath();
	char fullPath[500];
	_chdir("../OpenGL/data/video/");
	if (previousVideoFolder.compare(".") == 0)
		previousVideoFolder = std::string(_fullpath(fullPath, ".", 500));
	//Ask for avi file name
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Avi video file (*.avi)|*.avi|All files (*.*)|*.*||"));
	dlg.m_ofn.lpstrInitialDir = previousVideoFolder.c_str();
	if(dlg.DoModal() == IDCANCEL) {
		visageFolderManager.popPath();
		return;
	}
	previousVideoFolder = "";
	// return to current path after file browser dialog is closed, otherwise the necessary files may not be found
	visageFolderManager.popPath();
	std::string strAviFile = (std::string)dlg.GetPathName();
	//set the resulting .fba file name to the original .avi file name (just change the extension to .fba)
	std::string fbaFileName = strAviFile;
	fbaFileName.erase(fbaFileName.find_last_of(".")+1);
	fbaFileName.append("fba");

	std::string title = "Tracking from video: ";
	title.append(strAviFile.substr(strAviFile.find_last_of("\\")+1));
	SetTitle(title.c_str());

	if (writeDataEnabled) {
		// open data file, named same as video file but with txt extension
		std::string datafilename = strAviFile;
		datafilename.erase(datafilename.find_last_of(".")+1);
		datafilename.append("txt");
		datafile = fopen(datafilename.c_str(), "w");
	}

	videoFileName = strAviFile;

	// set the configuration file
	CString strName;
	m_MainWnd->GetConfigFilename(strName);
	m_Tracker->setTrackerConfiguration(strName.GetBuffer());

	m_OpenGLWnd->trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(trackFromVideo,0,(void*)this);
}

void CVisionExampleDoc::trackFromCAM(void *thisdoc)
{
	CVisionExampleDoc *td = (CVisionExampleDoc *)thisdoc;

	int cam_device = 0;
	int cam_width = 800;
	int cam_height = 600;
	int cam_fps = 30;
	int format = VISAGE_FRAMEGRABBER_FMT_RGB;

	CString strCamRes;
	td->m_MainWnd->GetCameraResolution(strCamRes);
	sscanf((LPCTSTR)strCamRes, "%dx%d", &cam_width, &cam_height);

	cam_device = td->cam_device = td->m_MainWnd->GetCameraDevice();

#define USE_VIDEOINPUT

#ifdef USE_VIDEOINPUT

	// Grabbing from camera using VideoInput library
	videoInput *VI = td->VI = new videoInput();

	VI->setIdealFramerate(cam_device, cam_fps);
	bool r = VI->setupDevice(cam_device, cam_width, cam_height);
	cam_width = VI->getWidth(cam_device);
	cam_height = VI->getHeight(cam_device);

	printf("Start tracking from camera %dx%d@%d\n", cam_width, cam_height, cam_fps);

	IplImage *framePtr = cvCreateImage(cvSize(cam_width, cam_height), IPL_DEPTH_8U, 3);

	int max_faces = td->m_MainWnd->GetMaxFaces();

	FaceData trackingData[10];

	int *track_stat = 0;

	while (td->m_OpenGLWnd->trackerActive) {

		while(!VI->isFrameNew(cam_device))
			Sleep(1);

		VI->getPixels(cam_device, (unsigned char*)framePtr->imageData, false, true);
#else
	// Grabbing from camera using OpenCV library
	CvCapture* capture = cvCaptureFromCAM( cam_device );

	int ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, cam_width);
	ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, cam_height);
	ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, cam_fps);

	cam_width = (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );
	cam_height = (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );

	while (td->m_OpenGLWnd->trackerActive && cvGrabFrame(capture)) {

		const IplImage *framePtr = cvRetrieveFrame(capture); // grab latest frame
#endif
		startTime = getCurrentTimeMs();
		track_stat = td->m_Tracker->track(
			framePtr->width, framePtr->height, framePtr->imageData,
			trackingData, format, framePtr->origin, framePtr->widthStep, -1, max_faces);
		endTime = getCurrentTimeMs();
		elapsedTime = endTime - startTime;

		displayResults(td, track_stat, trackingData, (VsImage*)framePtr, (VsImage*)logo, max_faces);

		if (td->writeDataEnabled) td->writeResults(track_stat[0], trackingData[0]);
	}

	td->m_Tracker->stop();

#ifdef USE_VIDEOINPUT
	VI->stopDevice(cam_device);
	delete VI; td->VI = VI = 0;
	cvReleaseImage(&framePtr);
#else
	cvReleaseCapture(&capture);
#endif

	printf("Stop tracking from camera.\n");

	td->finishWriteResults();

	_endthread();
}

/**
* The main function for tracking from a video camera connected to the computer.
* It is called when the Tracking->Track from Cam menu item is chosen.
*
*
* @see OnTrackingTrackfromavivideo()
*/
void CVisionExampleDoc::OnTrackingTrackfromcam()
{
	// stop thread if active previously
	m_OpenGLWnd->trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	if (writeDataEnabled) {
		// open data file, named cam_data-HH-MM-SS_DD_MM_YY.txt
		SYSTEMTIME systemTime;
		GetSystemTime(&systemTime);

		char datafilename[50];
		sprintf(datafilename, "cam_data-%02d-%02d-%02d_%02d-%02d-%02d.txt", systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wDay, systemTime.wMonth, systemTime.wYear);
		datafile = fopen(datafilename, "w");
	}

	SetTitle("Tracking from camera");

	// set the configuration file
	CString strName;
	m_MainWnd->GetConfigFilename(strName);
	m_Tracker->setTrackerConfiguration(strName.GetBuffer());

	m_OpenGLWnd->trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(trackFromCAM,0,(void*)this);
}

void CVisionExampleDoc::OnTrackingCamerasettings()
{
	bool ok = false;

	if(VI) {
		VI->showSettingsWindow(cam_device);
		ok = true;
	}

	if(!ok)
		MessageBox(0,"Camera settings can be changed only when camera is active, i.e. while tracking from camera.", "Can not open camera settings",
			MB_OK | MB_ICONWARNING );
}

void CVisionExampleDoc::trackFromImage(void *thisdoc)
{
	CVisionExampleDoc *td = (CVisionExampleDoc *)thisdoc;

	// get the current path
	visageFolderManager.pushPath();
	//Ask for image file name
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Image files (*.jpg;*.png)|*.jpg;*.png|All files (*.*)|*.*||"));
	dlg.m_ofn.lpstrInitialDir = previousImageFolder.c_str();
	if(dlg.DoModal() == IDCANCEL) {
		visageFolderManager.popPath();
		_endthread();
	}
	previousImageFolder = "";
	// return to current path after file browser dialog is closed, otherwise the necessary files may not be found
	visageFolderManager.popPath();
	std::string strImgFile = (std::string)dlg.GetPathName();
	if(td->testImage) cvReleaseImage(&td->testImage);
	td->testImage = cvLoadImage(strImgFile.c_str());

	if(!td->testImage) {
		MessageBox(0,"Bad image format!","Error",MB_ICONERROR);
		_endthread();
	}

	std::string title = "Tracking from image: ";
	title.append(strImgFile.substr(strImgFile.find_last_of("\\")+1));
	//td->SetTitle(title.c_str());

	if (td->writeDataEnabled) {
		// open data file, named same as image file but with txt extension
		std::string datafilename = strImgFile;
		datafilename.erase(datafilename.find_last_of(".")+1);
		datafilename.append("txt");
		td->datafile = fopen(datafilename.c_str(), "w");
	}

	// set the configuration file
	CString strName;
	td->m_MainWnd->GetConfigFilename(strName);
	td->m_Tracker->setTrackerConfiguration(strName.GetBuffer());

	int format;
	switch(td->testImage->nChannels)
	{
		case 3:
			format = VISAGE_FRAMEGRABBER_FMT_BGR;
			break;
		case 1:
			format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
			break;
		default:
			MessageBox(0,"Unsupported image format.","Error",MB_ICONERROR);
			_endthread();
	}

	int max_faces = td->m_MainWnd->GetMaxFaces();

	FaceData trackingData[10];

	int *track_stat = 0;

	printf("Start tracking from image %s\n", strImgFile.c_str());

	const IplImage *framePtr = td->testImage;

	while (td->m_OpenGLWnd->trackerActive) {

		startTime = getCurrentTimeMs();
		track_stat = td->m_Tracker->track(
			framePtr->width, framePtr->height, framePtr->imageData,
			trackingData, format, framePtr->origin, framePtr->widthStep, -1, max_faces);
		endTime = getCurrentTimeMs();
		elapsedTime = endTime - startTime;

		displayResults(td, track_stat, trackingData, (VsImage*)framePtr, (VsImage*)logo, max_faces);
	
		if (td->writeDataEnabled) td->writeResults(track_stat[0], trackingData[0]);
	}

	td->m_Tracker->stop();

	cvReleaseImage(&td->testImage);

	printf("Stop tracking from image.\n");

	td->finishWriteResults();

	_endthread();
}

/**
* The main function for tracking in a static image.
* The main purpose is to demonstrate the raw image interface for the tracker.
*
*
* @see OnTrackingTrackfromavivideo()
*/
void CVisionExampleDoc::OnTrackingTrackinimage()
{	
	// stop thread if active previously
	m_OpenGLWnd->trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	m_OpenGLWnd->trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(trackFromImage,0,(void*)this);
}

void CVisionExampleDoc::testingThread(void *thisdoc)
{
	CVisionExampleDoc *td = (CVisionExampleDoc *)thisdoc;

	// set the configuration file
	CString strName;
	td->m_MainWnd->GetConfigFilename(strName);
	td->m_Tracker->setTrackerConfiguration(strName.GetBuffer());

	td->finishWriteResults();

	visageFolderManager.pushPath();
	char fullPath[500];
	_chdir("../OpenGL/data/FaceDetector");
	if (previousBatchFolder.compare(".") == 0)
		previousBatchFolder = std::string(_fullpath(fullPath, ".", 500));
	CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("txt (*.txt)|*.txt|All files (*.*)|*.*||"));
	dlg.m_ofn.lpstrInitialDir = previousBatchFolder.c_str();
	if(dlg.DoModal() == IDCANCEL) {
		visageFolderManager.popPath();
		_endthread();
	}
	previousBatchFolder = "";
	FILE *listFile = fopen((const char*)dlg.GetPathName(),"r");
	if (!listFile)
		_endthread();
	char inString[200];
	char fileName[200];
	char rootFolder[200];
	bool ignore_spec_commands = false;
	strcpy(rootFolder, (const char*)dlg.GetFolderPath());
	//_getcwd(rootFolder,200);
	visageFolderManager.popPath();

	while(td->m_OpenGLWnd->trackerActive && fgets(inString,200,listFile))
	{
		if(inString[strlen(inString)-1]==10) inString[strlen(inString)-1]=0; //clip Line Feed character from the end
		if(strlen(inString)==0) continue; //skip empty lines
		if(inString[0]=='#') continue; //skip comments

		if(inString[0]=='$') //process special commands
		{
			//input folder
			if(strstr(inString,"$START:")) startFrame = atoi(inString+7);
			if(strstr(inString,"$END:")) endFrame = atoi(inString+5);
			if(strstr(inString,"$SLOWDOWN_START:")) slowdownFrame = atoi(inString+16);
			if(strstr(inString,"$SLOWDOWN_TIME:")) slowdownTime = atoi(inString+15);
			if(strstr(inString,"$IGNORE_SPECIAL_COMMANDS:")) ignore_spec_commands = (atoi(inString+25) != 0);
			continue;
		}

		if(ignore_spec_commands)
		{
			startFrame = -1;
			endFrame = -1;
			slowdownFrame = -1;
			slowdownTime = -1;
		}
		sprintf(fileName,"%s\\%s",rootFolder,inString);

		if (td->writeDataEnabled) {
			// open data file, named same as video file but with txt extension
			std::string datafilename = fileName;
			datafilename.erase(datafilename.find_last_of(".")+1);
			datafilename.append("txt");
			td->datafile = fopen(datafilename.c_str(), "w");
		}

		td->videoFileName = fileName;

		td->m_OpenGLWnd->trackerActive = true;
		
		HANDLE videoThreadHandle = (HANDLE)_beginthread(trackFromVideo,0,(void*)td);

		WaitForSingleObject(videoThreadHandle, INFINITE);

		startFrame = -1;
		endFrame = -1;
		slowdownFrame = -1;
		slowdownTime = -1;

		td->finishWriteResults();
	}

	fclose(listFile);

	_endthread();
}


void CVisionExampleDoc::OnTrackingBatchprocessing()
{
	// stop thread if active previously
	m_OpenGLWnd->trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	m_OpenGLWnd->trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(testingThread,0,(void*)this);
}


void CVisionExampleDoc::OnTrackingWritedata() 
{
	// Get the popup menu which contains the "Toggle State" menu item.
	CMenu* mmenu = m_MainWnd->GetMenu();
	CMenu* submenu = mmenu->GetSubMenu(1);

	// Check the state of the "Toggle State" menu item. Check the menu item
	// if it is currently unchecked. Otherwise, uncheck the menu item
	// if it is not currently checked.
	UINT state = submenu->GetMenuState(ID_TRACKING_WRITEDATA, MF_BYCOMMAND);
	ASSERT(state != 0xFFFFFFFF);

	if (state & MF_CHECKED) {
		submenu->CheckMenuItem(ID_TRACKING_WRITEDATA, MF_UNCHECKED | MF_BYCOMMAND);
		writeDataEnabled = false;
	}
	else {
		submenu->CheckMenuItem(ID_TRACKING_WRITEDATA, MF_CHECKED | MF_BYCOMMAND);
		writeDataEnabled = true;
	}
}