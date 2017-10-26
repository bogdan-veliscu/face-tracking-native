#import <Foundation/Foundation.h>
#import "CustomGLView.h"
#import "Informer.h"

// declare C++ implementation for .mm (Obj-C++) files
#ifdef __cplusplus
#include "visageVision.h"
#include "DemoFrameGrabber.h"
#include "OSXCameraGrabber.h"
#include "CameraGrabber.h"
#include "OSXCapture.h"
#include "OpenGLInterface.h"

// neccessary prototype declaration for licensing
namespace VisageSDK
{
void initializeLicenseManager(const char *licenseKeyFileFolder);
}
#endif

// declare C++ implementation for .m (Obj-C) files
#ifdef __OBJC__
#ifndef __cplusplus
typedef void VisageTracker;
typedef void FaceData;
typedef void DemoFrameGrabber;
typedef void FDP;
typedef void VsImage;
#endif
#endif

/**
 * Implements simple high-level Objective-C interface around VisageSDK::VisageTracker functionallity.
 *
 * This makes connecting the iOS app GUI with visage|SDK tracker functionaly easier as primary programming language used in iOS apps is
 * Objective-C while primary programming language used in visage|SDK is C++. The class is a subclass of NSObject and
 * it uses Objective-C++ for implementation details.
 *
 * The sample application automatically chooses appropriate tracker configuration file based on device it is running on.
 *
 * Key members of this class are:
 * - tracker: the VisageTracker object
 * - trackingData: the TrackingData object used for retrieving and holding tracking data
 * - glView: the OpenGL view object used for setting and displaying video frame and tracking results
 * - displayTrackingResults: method that demonstrates how to acquire, use and display tracking data and 3D face model
 * 
 */
@interface TrackerWrapper : NSObject {
    /**
     * View used for rendering.
     */
	CustomGLView *glView;
    /**
     * Informer used for displaying tracking information.
     */
    Informer* informer;
    #ifdef __cplusplus
	VisageSDK::VisageTracker* tracker;
    VisageSDK::FaceData trackingData[4];
    DemoFrameGrabber *demoFrameGrabber;
    VsImage* m_frame;
    OpenGLInterface* glInterface;
    
    /**
     * 
     */
	bool isTracking;
	
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
     *
     */
    bool inGetTrackingResults;
#endif
}

@property (nonatomic, retain) CustomGLView *glView;

/**
 * Method for initializing the tracker.
 *
 * This method creates a new VisageTracker objects and initializes tracking by choosing appropriate configuraion file based on device it is running on.
 */
- (void)initTracker:(CustomGLView *)view withConfigurationFile:(NSString*) configurationFile andInformer:(Informer*) info;
/**
 * Method for starting tracking from a camera.
 */
- (void)startTrackingFromCam;
/**
 * Method for starting tracking from a video.
 *
 * @param filename name of the video file
 */
- (void)startTrackingFromVideo:(NSString *)filename;
/**
 * Method for starting tracking from an image.
 */
- (void)startTrackingFromImage;
/**
 * Method for initializing the camera and performing face tracking on the given frame.
 */
- (void) trackingCamThread:(id)object;
/**
 * Method for loading picture and performing face tracking on the given frame.
 */
- (void) trackingImageThread:(id)object;
/**
 * Method for loading video file and performing face tracking on the given frame.
 */
- (void) trackingVideoThread: (NSString *)filename;
/**
 * Method for stoping the tracker.
 */
- (void)stopTracker;
/**
 * Function used for measuring tracker speed. Used for video synchronization (if flag video_file_sync is set to 1).
 */
- (long) getCurrentTimeMs;
/**
 * Method for displaying tracking results.
 *
 * This method is periodically called by the app timer to get and display tracking results. The results are retrieved via global VisageSDK::TrackingData object function and displayed
 * using labels for textual data and OpenGL ES for visual data (frames from camera and 3D face model). It shows how to properly interpret tracking data and set up the
 * OpenGL scene to display 3D face model retrieved from the tracker correctly aligned to the video frame.
 *
 * @param trackerStatus tracker status
 * @param frame frame to be drawn
 */
- (void)displayTrackingResults:(int*)trackerStatus Frame:(VsImage*)frame;

@end
