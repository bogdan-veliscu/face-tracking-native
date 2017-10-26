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

#ifndef __OSXCapture_h__
#define __OSXCapture_h__

#include "cv.h"

#define OSX_DEFAULT_FPS 30
#define OSX_DEFAULT_WIDTH 800
#define OSX_DEFAULT_HEIGHT 600

@class VideoGrabber;
@class CameraGrabber;

namespace VisageSDK
{

/** OSXCapture class, wrapper class for capturing frames from camera and video files on OSX
 *
 */
class OSXCapture{
    
private:
    bool checkIfFPSIsSupported(int fps);
public:
		
	/** Constructor.
	 *	
	 *	@param fps limits FPS of camera input
	 *	@param width sets desired width of frame for camera input
	 */
	OSXCapture(int fps = OSX_DEFAULT_FPS, int width = OSX_DEFAULT_WIDTH, int height = OSX_DEFAULT_HEIGHT);
		
	/** Destructor.
	 *	
	 */
	virtual ~OSXCapture(void);

	int getFramerate(void);
	void setIdealFramerate(int framerate);
	
	void setupCapture(int width, int height);
    
    virtual bool isTracking();
    virtual void startTracking() = 0;
    virtual void stopTracking() = 0;
	
	int getWidth(void);
	int getHeight(void);
	
	void setWidth(int width);
    void setHeight(int height);

protected:
    bool _isTracking;
    
private:
	int _framerate;
	int _width;
	int _height;
};
	
}
	
#endif // __OSXCapture_h__