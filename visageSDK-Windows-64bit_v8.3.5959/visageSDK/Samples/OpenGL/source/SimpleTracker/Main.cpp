#include <stdio.h>
#include <sys/timeb.h>
#include <process.h> 

#include "visageVision.h"
#include "FolderManager.h"
#include "StrStreamUtils.h"

#include "videoInput.h"
#include "highgui.h"

using namespace std;
using namespace VisageSDK;

static VisageTracker *m_Tracker = 0;
static HANDLE trackingThreadHandle = 0;
static bool trackerActive = false;
static const char* videoFileName = 0;

/**
* Function for clearing console window. 
*/
int myClrScrn(void)
{
	// Top left corner coordinates
	COORD coord = {0};           

	// Contains information about screen buffer     
	CONSOLE_SCREEN_BUFFER_INFO cBufferInfo;    

	// Characters written in buffer
	DWORD dwI;        
			
	// Characters to clear ("area" of console, so to speak)
	DWORD dwSize;                

	// Handle to console
	HANDLE hI = GetStdHandle(STD_OUTPUT_HANDLE); 
	
	// Just a bit error handling
	if(hI == INVALID_HANDLE_VALUE) return (EXIT_FAILURE); 

	// GetConsoleScreenBufferInfo returns 0 on error
	if (GetConsoleScreenBufferInfo(hI, &cBufferInfo))      
	{
		// Calculate characters to clear
		dwSize = cBufferInfo.dwSize.X * cBufferInfo.dwSize.Y;

		// Write space characters to buffer as many times as needed, i.e. dwSize    
		FillConsoleOutputCharacter(hI, TEXT(' '), dwSize, coord, &dwI );

		// Set console cursor to top left coord
		SetConsoleCursorPosition(hI, coord); 
	
		return (EXIT_SUCCESS);
	} else
		return (EXIT_FAILURE);
}

static void DrawResults(FaceData &trackingData, VsImage* frame, int track_stat)
{
	myClrScrn();
	printf("Tracking in progress!\nPress Escape key to quit.\n\n");

	float tracking_quality = trackingData.trackingQuality;

	char *tstatus;
	switch (track_stat) {
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

	printf(" Tracking status: %s\n\n", tstatus);

	static float eyeDistance = -1.0f;

	// Example code for accessing specific feature points, in this case nose tip and eyes
	if(track_stat == TRACK_STAT_OK)
	{
		FDP *fp = trackingData.featurePoints3DRelative;
		float nosex = fp->getFPPos(9,3)[0];
		float nosey = fp->getFPPos(9,3)[1];
		float nosez = fp->getFPPos(9,3)[2];
		float lex = fp->getFPPos(3,5)[0];
		float ley = fp->getFPPos(3,5)[1];
		float lez = fp->getFPPos(3,5)[2];
		float rex = fp->getFPPos(3,6)[0];
		float rey = fp->getFPPos(3,6)[1];
		float rez = fp->getFPPos(3,6)[2];
		float eyedist = lex -rex;
		int dummy = 0;
	}

	// display the frame rate, position and other info
	float trackingFrameRate = trackingData.frameRate;
	float r1[3];
	float t1[3];
	if(track_stat == TRACK_STAT_OK)
	{
		for(int i=0;i<3;i++)
			r1[i] = (180.0f * trackingData.faceRotation[i]) / 3.14f;
		for(int i=0;i<3;i++)
			t1[i] = trackingData.faceTranslation[i] * 100.0f; //translation is expressed in meters so this gives approximate values in centimeters if camera focus distance parameter f in fbft is set correctly
	}
	else
	{
		for(int i=0;i<3;i++)
			r1[i] = 0.0f;
		for(int i=0;i<3;i++)
			t1[i] = 0.0f; 
	}

	if (track_stat != TRACK_STAT_OFF)
		printf(" %4.1f FPS \n Head position %+5.1f %+5.1f %+5.1f \n Rotation (deg) %+5.1f %+5.1f %+5.1f \n Resolution: input %dx%d\n\n",trackingFrameRate,t1[0],t1[1],t1[2],r1[0],r1[1],r1[2],frame->width, frame->height);
}

static long getCurrentTimeMs(bool init = false)
{
	static long initialTime = 0;

	struct _timeb timebuffer;
	_ftime( &timebuffer );

	long clockTime = 1000 * (long)timebuffer.time + timebuffer.millitm;	//CHANGED BY I.K. added explicit cast

	if(init)
		initialTime = clockTime;
	
	clockTime -= initialTime;
	
	return clockTime;
}

void trackFromVideo(void *ptr)
{
	VisageTracker *tracker = (VisageTracker *)ptr;

	int* track_stat = TRACK_STAT_OFF;

	int format = VISAGE_FRAMEGRABBER_FMT_BGR;

	bool video_file_sync = true;

	CvCapture* capture = cvCaptureFromFile( videoFileName );

	getCurrentTimeMs(true);

	float frameTime = 1000.0f/(float)cvGetCaptureProperty( capture, VS_CAP_PROP_FPS );
	int frameCount = 0;

	FaceData faceData;

	while (trackerActive && cvGrabFrame(capture)) {

		long currentTime = getCurrentTimeMs();

		while(video_file_sync && currentTime > frameTime*(frameCount+1) ) // skip frames if track() is too slow
		{
			if(!trackerActive || !cvGrabFrame(capture))
				break;

			frameCount++;
			currentTime = getCurrentTimeMs();
		}

		while(video_file_sync && currentTime < frameTime*(frameCount-5)) // wait if track() is too fast
		{
			Sleep(1);
			currentTime = getCurrentTimeMs();

			if(!trackerActive)
				break;
		}

		const IplImage *framePtr = cvRetrieveFrame(capture); // grab latest frame

		frameCount++;

		track_stat = tracker->track(framePtr->width,framePtr->height,framePtr->imageData, &faceData, format, framePtr->origin, framePtr->widthStep, (long)frameTime*frameCount);

		DrawResults(faceData, (VsImage*)framePtr, track_stat[0]);
	}

	track_stat = tracker->track(0,0,0,0);
	cvReleaseCapture(&capture);

	printf("Stop tracking from video.\n");

	_endthread();
}

/**
* The main function for tracking from a video file. 
* It is called when the 'v' is chosen.
*
*/
void trackfromavivideo(char *strAviFile, char *cfgFile)
{
	trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	// set the configuration file
	m_Tracker->setTrackerConfiguration(cfgFile);

	videoFileName = strAviFile;

	trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(trackFromVideo,0,(void*)m_Tracker);
}

void trackFromCAM(void *ptr)
{
	VisageTracker *tracker = (VisageTracker *)ptr;

	int* track_stat = TRACK_STAT_OFF;

	int cam_device = 0;
	int cam_width = 800;
	int cam_height = 600;
	int cam_fps = 30;
	int format = VISAGE_FRAMEGRABBER_FMT_RGB;

#define USE_VIDEOINPUT
#ifdef USE_VIDEOINPUT
	// ***** Grabbing from camera using Videoinput library *******
	videoInput *VI = new videoInput();

	VI->setIdealFramerate(cam_device, cam_fps);
	bool r = VI->setupDevice(cam_device, cam_width, cam_height);
	cam_width = VI->getWidth(cam_device);
	cam_height = VI->getHeight(cam_device);

	IplImage *framePtr = cvCreateImage(cvSize(cam_width, cam_height), IPL_DEPTH_8U, 3);

	FaceData faceData;

	while (trackerActive) {

		while(!VI->isFrameNew(cam_device))
			Sleep(1);

		VI->getPixels(cam_device, (unsigned char*)framePtr->imageData, false, true);
#else
	// ***** Grabbing from camera using OpenCV library *******
	CvCapture* capture = cvCaptureFromCAM( cam_device );

	int ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, cam_width);
	ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, cam_height);
	ret = cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, cam_fps);

	cam_width = (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );
	cam_height = (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );

	while (trackerActive && cvGrabFrame(capture)) {

		const IplImage *framePtr = cvRetrieveFrame(capture); // grab latest frame
#endif

		track_stat = tracker->track(framePtr->width,framePtr->height,framePtr->imageData, &faceData, format, framePtr->origin);

		DrawResults(faceData, (VsImage*)framePtr, track_stat[0]);
	}

	track_stat = tracker->track(0,0,0,0);

#ifdef USE_VIDEOINPUT
	VI->stopDevice(cam_device);
	delete VI; VI = 0;
	cvReleaseImage(&framePtr);
#else
	cvReleaseCapture(&capture);
#endif

	_endthread();
}

/**
* The main function for tracking from a video camera connected to the computer.
* It is called when the 'c' is chosen.
*
*/
void trackfromcam(char *cfgFile)
{
	// stop thread if active previously
	trackerActive = false;
	
	WaitForSingleObject(trackingThreadHandle, INFINITE);

	// set the configuration file
	m_Tracker->setTrackerConfiguration(cfgFile);

	trackerActive = true;

	trackingThreadHandle = (HANDLE)_beginthread(trackFromCAM,0,(void*)m_Tracker);
}

/*! \mainpage  visage|SDK SimpleTracker example project
*
* \if WIN64_DOXY
* \htmlonly
* <table border="0">
* <tr>
* <td width="32"><a href="../../../../../../bin64/DEMO_SimpleTracker_x64.exe"><img src="../../../../../../doc/OpenGL/images/run_sample.png" border=0 title="Run Sample (this may not work in all browsers, in such case please run DEMO_FaceTracker2_x64.exe from the visageSDK\bin64 folder.)"></a></td>
* <td width="32"><a href="../../../../../../bin64"><img src="../../../../../../doc/OpenGL/images/open_bin_folder.png" border=0 title="Open Binary Folder (visageSDK\bin64)"></a></td>
* <td width="32"><a href="../../../../data/FaceTracker2"><img src="../../../../../../doc/OpenGL/images/open_data_folder.png" border=0 title="Open Data Folder"></a></td>
* <td width="32"><a href="../../../../data/video"><img src="../../../../../../doc/OpenGL/images/open_folder.png" border=0 title="Open Video Folder"></a></td>
* <td width="32"><a href="../../../../build/msvc140/SimpleTracker"><img src="../../../../../../doc/OpenGL/images/open_project_folder.png" border=0 title="Open Project Folder"></a></td>
* <td width="32"><a href="../../../../source/SimpleTracker"><img src="../../../../../../doc/OpenGL/images/open_source_folder.png" border=0 title="Open Source Code Folder"></a></td>
* </tr>
* </table>
* \endhtmlonly
* \else
* \htmlonly
* <table border="0">
* <tr>
* <td width="32"><a href="../../../../../../bin/DEMO_SimpleTracker.exe"><img src="../../../../../../doc/OpenGL/images/run_sample.png" border=0 title="Run Sample (this may not work in all browsers, in such case please run DEMO_FaceTracker2.exe from the visageSDK\bin folder.)"></a></td>
* <td width="32"><a href="../../../../../../bin"><img src="../../../../../../doc/OpenGL/images/open_bin_folder.png" border=0 title="Open Binary Folder (visageSDK\bin)"></a></td>
* <td width="32"><a href="../../../../data/FaceTracker2"><img src="../../../../../../doc/OpenGL/images/open_data_folder.png" border=0 title="Open Data Folder"></a></td>
* <td width="32"><a href="../../../../data/video"><img src="../../../../../../doc/OpenGL/images/open_folder.png" border=0 title="Open Video Folder"></a></td>
* <td width="32"><a href="../../../../build/msvc140/SimpleTracker"><img src="../../../../../../doc/OpenGL/images/open_project_folder.png" border=0 title="Open Project Folder"></a></td>
* <td width="32"><a href="../../../../source/SimpleTracker"><img src="../../../../../../doc/OpenGL/images/open_source_folder.png" border=0 title="Open Source Code Folder"></a></td>
* </tr>
* </table>
* \endhtmlonly
* \endif
*
* The SimpleTracker example project demonstrates the following capabilities of visage|SDK:
* - real-time head/facial features tracking from video file or camera based on the VisageSDK::VisageTracker tracker
*
* This is an intentionally simpliefied version of a tracking application, intended to be used by developers as 
* an introduction. SimpleTracker is a console application, with keyboard based choices made by the
* user. It then starts tracking and displays tracking status and results in the console. There is no video display.
*
* A more complete tracker sample project is the FaceTracker2 sample project.
*
* Classes and methods that are important for detailed understanding how this
* sample project is implemented and how it used visage|SDK are:
* - main(): implements the main user interaction.
*/

/** @file */

/*! The simple tracker.
*/
int main( int argc, const char* argv[] )
{
	char *videofilename = "../OpenGL/data/video/kinoP_clip.avi";
	char *cfgfile = NULL;
	char source, type;

	visageFolderManager.init();

	char defaultConfigFile[100];
	FILE *f = fopen("DefaultConfiguration.txt","r");
	if (f == NULL) {
		printf("Error opening default configuration file!\n");
		exit(1);
	}
	fgets(defaultConfigFile,100,f);
	fclose(f);
	defaultConfigFile[strlen(defaultConfigFile)-1] = 0; //remove line feed character

	m_Tracker = new VisageTracker(defaultConfigFile); 

	printf("Simple Tracker application\n\n");

	printf("Track from (v)ideo or (c)am? ");
	scanf("%c", &source);

	printf("Type of tracking (h)ead, (f)acial features or (d)efault? ");
	scanf(" %c", &type);

	switch (type) {
		case 'h':
			cfgfile = "Head Tracker.cfg";
			break;
		case 'f':
			cfgfile = "Facial Features Tracker - High.cfg";
			break;
		default:
			cfgfile = defaultConfigFile;
			break;
	}

	switch (source) {
		case 'v':
			trackfromavivideo(videofilename, cfgfile);
			break;
		case 'c':
			trackfromcam(cfgfile);
			break;
		default:
			printf("Wrong choice!\n");
			break;
	}

	// Waiting while tracker is tracking
	while (trackerActive) {
		if (GetAsyncKeyState(VK_ESCAPE) & 0x01) {
			trackerActive = false;
			WaitForSingleObject(trackingThreadHandle, INFINITE);
		}
		Sleep(100);
	}

	m_Tracker->stop();

	delete m_Tracker;

	return 0;
}
