
#include "visageVision.h"

using namespace VisageSDK;

/**
 * Implements a simple frame grabber that serves a single image to the tracker in order to
 * demonstrate the raw image input mechanism. The image is read from a file.
 * 
 */
class DemoFrameGrabber{
public:
	
    /**
     * Contructor.
     *
     * Initialises the frame grabber by reading an image from a file.
     * 
     */
	DemoFrameGrabber(void);
    
    /**
     * Destructor.
     * 
     * Releases the frame.
     *
     */
    ~DemoFrameGrabber(void);
    
    /**
     * Frame that is served to the tracker.
     * 
     */
    VsImage *frame;
    
    int width;
    int height;
    int format;
    int origin;

    /**
	 * 
	 * VisageTracker calls this function periodically to get the new frame to process.
     * In this simple implementation same frame is returned every time.
	 */
	unsigned char *GrabFrame(long &timeStamp);
};