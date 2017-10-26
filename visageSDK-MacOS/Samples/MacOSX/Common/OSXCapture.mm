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

#include "OSXCapture.h"
#import "CameraGrabber.h"

namespace VisageSDK
{
    OSXCapture::OSXCapture(int fps, int width, int height)
    {
        _width = width;
        _height = height;
        _framerate = checkIfFPSIsSupported(fps) ? fps : OSX_DEFAULT_FPS;
        _isTracking = false;
    }
	
    OSXCapture::~OSXCapture(void)
    {
    }

    bool OSXCapture::checkIfFPSIsSupported(int fps)
    {
        return !(fps > 30);
    }

    int OSXCapture::getFramerate(void)
    {
        return this->_framerate;
    }

    void OSXCapture::setIdealFramerate(int framerate)
    {
        this->_framerate = framerate;
    }
	
    void OSXCapture::setupCapture(int width, int height)
    {
        this->_width = width;
        this->_height = height;
    }

    int OSXCapture::getWidth(void)
    {
        return this->_width;
    }

    void OSXCapture::setWidth(int width)
    {
        _width = width;
    }

    int OSXCapture::getHeight(void)
    {
        return this->_height;
    }
    
    void OSXCapture::setHeight(int height)
    {
        _height = height;
    }
    
    bool OSXCapture::isTracking()
    {
        return _isTracking;
    }
}