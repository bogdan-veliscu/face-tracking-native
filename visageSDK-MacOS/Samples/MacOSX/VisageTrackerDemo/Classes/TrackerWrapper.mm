#import "TrackerWrapper.h"

#import "visageVision.h"
#import "DemoFrameGrabber.h"
#import "VisageRendering.h"

#import "HelperFunctions.h"

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <cv.h>
#include <highgui.h>
#include <cstdlib>
#include <sys/timeb.h>

#define V_PI 3.1415926535897932384626433832795 // PI
#define RAD2DEG(x) ( (x) * 180.f / V_PI ) // converts radians to degrees

using namespace VisageSDK;

@implementation TrackerWrapper

@synthesize glView;

static OSXCameraGrabber *OSXInput = 0;
static int frameWidth;
static int frameHeight;
int isMirrored = 1;
long fg_pts = -1;
unsigned int cam_device; //camera device number
int m_cam_device; //if -1, read cam_device number from configuration file, else take this number
unsigned int cam_width; //camera width
unsigned int cam_height; //camera height
static int *trackerStatus = TRACK_STAT_OFF;
const int max_faces = 4;
IplImage* frame_input;
bool isTracking=false;
static bool threadRunning = false;
static NSCondition *waitHandle;
CvCapture* capture;
char videoFile[500];
long currentTime;
long startTime;
int frameCount; // frame count from beginning of tracking
double frameTime; // duration of one frame in milliseconds

//pure track time variables
long startTrackTime = 0;
long endTrackTime = 0;
long elapsedTrackTime = 0;

//logo image
VsImage *logo = NULL;
int logoViewportWidth;
int logoViewportHeight;

void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height);
VsImage* CreateVsImageFromNSImage(NSImage *image, int nChannels);

int clamp(int x)
{
    unsigned y;
    return !(y=x>>8) ? x : (0xff ^ (y>>24));
}


// comment this to use head tracking configuration
#define FACE_TRACKER

- (void)initTracker:(CustomGLView *)view withConfigurationFile:(NSString *)configurationFile andInformer:(Informer *)info
{
    waitHandle = [NSCondition new];
    threadRunning = false;
    
    //initialize licensing
    //example how to initialize license key
    //initializeLicenseManager("license-key-filename.vlc");
    
    if (self->informer == NULL)
        self->informer = info;
    
	glView = view;
    demoFrameGrabber = 0;

    if (glInterface == NULL)
        glInterface = new OpenGLInterface(view);

    tracker = new VisageTracker([configurationFile UTF8String]);
    
    //get OpenGL context size
    glWidth = glView.bounds.size.width;
    glHeight = glView.bounds.size.height;

    inGetTrackingResults = false;
    
    //load logo image by the given path
    NSString *logoPath = [[NSBundle mainBundle] pathForResource:@"logo" ofType:@"png"];
    NSImage *logoImage = [[NSImage alloc] initWithContentsOfFile:logoPath];
    logo = CreateVsImageFromNSImage(logoImage, 4);
}


- (void) trackingCamThread:(id)object
{
    // if camera already works, release
    if (OSXInput != NULL) {
        if (OSXInput->isTracking())
          OSXInput->stopTracking();
        delete OSXInput;
        OSXInput = NULL;
        }
    
    // initialize new camera
    OSXInput = new OSXCameraGrabber(OSX_DEFAULT_FPS, OSX_DEFAULT_WIDTH, OSX_DEFAULT_HEIGHT, OSX_DEFAULT_DEVICE);
    OSXInput->startTracking();
    OSXInput->GrabFrame(fg_pts, isMirrored);
    
    frameWidth 	= OSXInput->getWidth();
    frameHeight = OSXInput->getHeight();
    
    int pixelFormat = OSXInput->getFormat();
    
    int format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
    
    if (pixelFormat == 1)
        format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
    else
        format = VISAGE_FRAMEGRABBER_FMT_BGRA;
        

    m_frame = vsCreateImage(vsSize(frameWidth, frameHeight), VS_DEPTH_8U, 4);
    
    while(isTracking)
    {
        unsigned char* pixels = OSXInput->GrabFrame(fg_pts, isMirrored);
       
        startTrackTime = [self getCurrentTimeMs];
        trackerStatus = tracker->track(frameWidth, frameHeight, (const char *)pixels, trackingData, format, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1, max_faces);
        endTrackTime = [self getCurrentTimeMs];
        elapsedTrackTime = endTrackTime - startTrackTime;
        
        if (pixelFormat == 1) {
            //Convert to RGBA for image to be drawn
            YUV_TO_RGBA(pixels, (unsigned char*)m_frame->imageData, frameWidth, frameHeight);
        }
        else {
            memcpy(m_frame->imageData, pixels, m_frame->imageSize);
        }
        
        //Display results
        [self displayTrackingResults:trackerStatus Frame:m_frame];
        
        int trackerCount = 0;
        
        for (int i = 0; i < max_faces;  i++)
        {
            if (trackerStatus[i] == TRACK_STAT_OFF)
                trackerCount++;
        }
        
        if (trackerCount == max_faces)
            break;
    }
    
    vsReleaseImage(&m_frame);
    
    [waitHandle lock];
    threadRunning = false;
    [waitHandle broadcast];
    [waitHandle unlock];
    
}


- (void)startTrackingFromCam
{
	[self stopTracker];
    
    while(inGetTrackingResults)
        ;

    [waitHandle lock];
    threadRunning = true;
    isTracking = true;
    [waitHandle broadcast];
    [waitHandle unlock];
    
    [NSThread detachNewThreadSelector:@selector(trackingCamThread:) toTarget:self withObject: self];
}


- (void) trackingVideoThread: (NSString *)filename
{
    double fps;
    bool video_file_sync = true;
    
    frameCount = 0;
    
    if(filename)
       {
           if(capture)
               cvReleaseCapture( &capture );
           
           strcpy(videoFile, [filename UTF8String]);
           capture = cvCaptureFromFile(videoFile);
           
           fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
           frameTime = 1000.0/fps;
           
           int width = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
           int height = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
           
           startTime = [self getCurrentTimeMs];
           
           m_frame = vsCreateImage(vsSize(width, height), VS_DEPTH_8U, 3);
           
           while(capture && isTracking && cvGrabFrame(capture))
           {
               if (video_file_sync)
               {
                   currentTime = [self getCurrentTimeMs];
                   
                   while((currentTime - startTime) > frameTime*(1+frameCount) )
                   {
                       if(!cvGrabFrame( capture )) //end of stream
                           break;
                       
                       frameCount++;
                       currentTime = [self getCurrentTimeMs];
                   }
                   
                   while((currentTime - startTime) < frameTime*(frameCount-5))
                   {
                       usleep(1000);
                       currentTime = [self getCurrentTimeMs];
                       
                   }
                   
               }
               
               frameCount++;
               
               IplImage* videoFrame = cvRetrieveFrame(capture);
               
               startTrackTime = [self getCurrentTimeMs];
               trackerStatus = tracker->track(videoFrame->width, videoFrame->height, (const char *)videoFrame->imageData, trackingData, VISAGE_FRAMEGRABBER_FMT_BGR, VISAGE_FRAMEGRABBER_ORIGIN_TL, videoFrame->widthStep, -1, max_faces);
               endTrackTime = [self getCurrentTimeMs];
               elapsedTrackTime = endTrackTime - startTrackTime;
               
               //Remove wierd padding from videoFrame for drawing
               vsCopy(videoFrame, m_frame);
               
               //Display results
               [self displayTrackingResults:trackerStatus Frame:m_frame];
               
               int trackerCount = 0;

               for (int i = 0; i < max_faces;  i++)
               {
                   if (trackerStatus[i] == TRACK_STAT_OFF)
                       trackerCount++;
               }
               
               if (trackerCount == max_faces)
                   break;
               
           }

           cvReleaseCapture( &capture );
       }
    
    vsReleaseImage(&m_frame);
    [waitHandle lock];
    threadRunning = false;
    [waitHandle broadcast];
    [waitHandle unlock];
    
}


- (void)startTrackingFromVideo:(NSString *)filename
{
	[self stopTracker];
    
    while(inGetTrackingResults)
        ;
    
    [waitHandle lock];
    threadRunning = true;
    isTracking = true;
    [waitHandle broadcast];
    [waitHandle unlock];
    
    [NSThread detachNewThreadSelector:@selector(trackingVideoThread: ) toTarget:self withObject: filename];
}


- (long) getCurrentTimeMs
{

    struct timeb timebuffer;
    ftime (&timebuffer);
    
    long clockTime = 1000 * (long)timebuffer.time + timebuffer.millitm;
    
    return clockTime;

}


- (void) trackingImageThread:(id)object
{
    unsigned char* pixels = demoFrameGrabber->GrabFrame(fg_pts);
    m_frame = vsCreateImageHeader(vsSize(demoFrameGrabber->width, demoFrameGrabber->height), VS_DEPTH_8U, 4);
    
    while(isTracking)
    {
        startTrackTime = [self getCurrentTimeMs];
        trackerStatus = tracker->track(demoFrameGrabber->width, demoFrameGrabber->height, (const char*)pixels, trackingData, VISAGE_FRAMEGRABBER_FMT_RGBA, VISAGE_FRAMEGRABBER_ORIGIN_TL, 0, -1, max_faces);
        endTrackTime = [self getCurrentTimeMs];
        elapsedTrackTime = endTrackTime - startTrackTime;
        
        vsSetImageData(m_frame, pixels, m_frame->widthStep);
        
        //Display results
        [self displayTrackingResults:trackerStatus Frame:m_frame];
        
        int trackerCount = 0;
        
        for (int i = 0; i < max_faces;  i++)
        {
            if (trackerStatus[i] == TRACK_STAT_OFF)
                trackerCount++;
        }
        
        if (trackerCount == max_faces)
            break;
        
    }
    
    vsReleaseImageHeader(&m_frame);
    [waitHandle lock];
    threadRunning = false;
    [waitHandle broadcast];
    [waitHandle unlock];
    
}


- (void)startTrackingFromImage
{
	[self stopTracker];
    
    while(inGetTrackingResults)
        ;
	
    try
    {
        [waitHandle lock];
        threadRunning = true;
        isTracking = true;
        [waitHandle broadcast];
        [waitHandle unlock];
        
        demoFrameGrabber = new DemoFrameGrabber();
        [NSThread detachNewThreadSelector:@selector(trackingImageThread:) toTarget:self withObject: self];
        
    }
    
    catch(int e)
    {
        return;
    }
}


- (void)stopTracker
{
    [waitHandle lock];
    
    isTracking = false;
    
    //get OpenGL context size
    glWidth = glView.bounds.size.width;
    glHeight = glView.bounds.size.height;
    
    // Keep waiting until either the predicate is true or we timed out
    while (threadRunning && [waitHandle waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:60]]) {
    } 
    
    [waitHandle unlock];
}

#include <mach/mach_time.h>

#define MEASURE_FRAMES 10
uint64_t last_times[MEASURE_FRAMES];
int framecount = -1;
int last_pts = 0;


-(void) displayTrackingResults:(int*)trackerStatus Frame:(VsImage*)frame
{
    inGetTrackingResults = true;
    
    int pts = (int)trackingData[0].timeStamp;
    
    if (last_pts == pts) {
        inGetTrackingResults = false;
        return;
    }
    
    last_pts = pts;
    
    mach_timebase_info_data_t timeBaseInfo;
    mach_timebase_info(&timeBaseInfo);
    
    //measure the frame rate
    uint64_t currentTime = mach_absolute_time() * timeBaseInfo.numer / (timeBaseInfo.denom * 1e6);
    if(framecount == -1)
    {
        framecount = 0;
        for(int i=0;i<10;i++)
            last_times[i]=0;
    }
    framecount++;
    if(framecount == MEASURE_FRAMES) framecount = 0;
    last_times[framecount] = currentTime;
    
    //drawing results
    glInterface->setOpenGLContext();
    
    //correct aspect
    videoAspect = frame->width / (double)frame->height;
    
    //
    int winWidth = glWidth;
    int winHeight = glWidth / videoAspect;
    
    //draw frame and tracking results
    VisageRendering::DisplayResults(&trackingData[0], trackerStatus[0], winWidth, winHeight, frame);
    
    for (int i = 1; i < max_faces; i++)
        VisageRendering::DisplayResults(&trackingData[i], trackerStatus[i], winWidth, winHeight, frame, DISPLAY_DEFAULT - DISPLAY_FRAME);
    
    //set logo viewport dimensions
    logoViewportWidth = winWidth;
    logoViewportHeight = winHeight;
    
    //draw logo
    if(logo != NULL)
        VisageRendering::DisplayLogo(logo, logoViewportWidth, logoViewportHeight);
    
    glInterface->swapOpenGLBuffers();

    const char *trackingStatusString;
    switch (trackerStatus [0]) {
        case TRACK_STAT_OFF:
            trackingStatusString = "OFF";
            break;
        case TRACK_STAT_OK:
            trackingStatusString = "OK";
            break;
        case TRACK_STAT_RECOVERING:
            trackingStatusString = "RECOVERING";
            break;
        case TRACK_STAT_INIT:
            trackingStatusString = "INITIALIZING";
            break;
        default:
            trackingStatusString = "N/A";
            break;
    }
    
    NSString* fps = [NSString stringWithFormat:@"%4.1f (track %ld ms)", trackingData[0].frameRate, elapsedTrackTime];
    NSString* info = [NSString stringWithFormat:@"Head position %+5.1f %+5.1f %+5.1f | Rotation (deg) %+5.1f %+5.1f %+5.1f", trackingData[0].faceTranslation[0] * 100.0f,
                      trackingData[0].faceTranslation[1] * 100.0f,
                      trackingData[0].faceTranslation[2] * 100.0f,
                      RAD2DEG(trackingData[0].faceRotation[0]),
                      RAD2DEG(trackingData[0].faceRotation[1]),
                      RAD2DEG(trackingData[0].faceRotation[2])];
    NSString* status = [NSString stringWithFormat:@"%s", trackingStatusString];
    
    [informer showTrackingFps:fps Info:info andStatus:status];
    
    
    inGetTrackingResults = false;
}


void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height)
{
    const int frameSize = width * height;
    
    const int ii = 0;
    const int ij = 0;
    const int di = +1;
    const int dj = +1;
    
    unsigned char* rgba = buff;
    
    for (int i = 0, ci = ii; i < height; ++i, ci += di)
    {
        for (int j = 0, cj = ij; j < width; ++j, cj += dj)
        {
            int y = (0xff & ((int) yuv[ci * width + cj]));
            int v = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 0]));
            int u = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 1]));
            y = y < 16 ? 16 : y;
            
            int a0 = 1192 * (y -  16);
            int a1 = 1634 * (v - 128);
            int a2 =  832 * (v - 128);
            int a3 =  400 * (u - 128);
            int a4 = 2066 * (u - 128);
            
            int r = (a0 + a1) >> 10;
            int g = (a0 - a2 - a3) >> 10;
            int b = (a0 + a4) >> 10;
            
            *rgba++ = clamp(r);
            *rgba++ = clamp(g);
            *rgba++ = clamp(b);
            *rgba++ = 255;
        }
    }
}


@end
