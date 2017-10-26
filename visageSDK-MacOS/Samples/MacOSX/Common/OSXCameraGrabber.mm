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

#import "OSXCameraGrabber.h"

namespace VisageSDK
{

OSXCameraGrabber::OSXCameraGrabber(int fps, int width, int height, int device) : VisageSDK::OSXCapture(fps, width, height){
    _rotated = width < height ? true : false;
    void (OSXCapture::*setSmallerDimension)(int) = _rotated ? &OSXCapture::setWidth: &OSXCapture::setHeight;
    void (OSXCapture::*setGreaterDimension)(int) = _rotated ? &OSXCapture::setHeight : &OSXCapture::setWidth;

    ((*this).*setSmallerDimension)(height);
    ((*this).*setGreaterDimension)(width);
    
    _cameraGrabber = [[CameraGrabber alloc] initWithWidth:getWidth() andHeight:getHeight() UseFPS:getFramerate() withDevice:device];
}

OSXCameraGrabber::~OSXCameraGrabber(void) {
    //[_cameraGrabber release];
}

void OSXCameraGrabber::startTracking() {
    [this->_cameraGrabber startCapture];
    //
    void (OSXCapture::*setSmallerDimension)(int) = _rotated ? &OSXCapture::setWidth: &OSXCapture::setHeight;
    void (OSXCapture::*setGreaterDimension)(int) = _rotated ? &OSXCapture::setHeight : &OSXCapture::setWidth;
    //
    ((*this).*setSmallerDimension)([_cameraGrabber frameHeight]);
    ((*this).*setGreaterDimension)([_cameraGrabber frameWidth]);
    _isTracking = true;
}

void OSXCameraGrabber::stopTracking() {
    [_cameraGrabber stopCapture];
    _isTracking = false;
}

unsigned char* OSXCameraGrabber::GrabFrame(long &time_stamp, int mirrored) {
    time_stamp = -1;
    return [_cameraGrabber getBuffer:_rotated isMirrored:mirrored];
}

int OSXCameraGrabber::getFormat()
{
    return [_cameraGrabber getPixelFormat];
}

void OSXCameraGrabber::getFrameProperties(int mirrored) {
    [_cameraGrabber startCapture];
    unsigned char* frame = [_cameraGrabber getBuffer:_rotated isMirrored:mirrored];
    setWidth(_cameraGrabber.frameWidth);
    setHeight(_cameraGrabber.frameHeight);
    [_cameraGrabber stopCapture];
    free(frame);
}

}
