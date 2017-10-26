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

#ifndef vsvision_OSXCameraGrabber_h
#define vsvision_OSXCameraGrabber_h

#ifdef __OBJC__
#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

#import "OSXCapture.h"
#import "CameraGrabber.h"
#else
#include "OSXCapture.h"

typedef void AVCaptureSession;
typedef void AVCaptureConnection;
typedef void CameraGrabber;
#endif

#include <pthread.h>

#define OSX_DEFAULT_DEVICE 0

namespace VisageSDK
{
    class OSXCameraGrabber : public OSXCapture {
    private:
        void getFrameProperties(int mirrored);
    public:
        OSXCameraGrabber(int fps = OSX_DEFAULT_FPS, int width = OSX_DEFAULT_WIDTH, int height = OSX_DEFAULT_HEIGHT, int device = OSX_DEFAULT_DEVICE);
        virtual ~OSXCameraGrabber(void);
        
        virtual void startTracking();
        virtual void stopTracking();
        virtual unsigned char* GrabFrame(long &time_stamp, int mirrored);
        
        int getFormat();
    
    private:       
        bool _rotated;
        
        AVCaptureSession *_captureSession;
        AVCaptureConnection *_videoConnection;
        AVCaptureVideoPreviewLayer *_prevLayer;
        
        NSString *_preset;
        
        CameraGrabber *_cameraGrabber;
    };
}

#endif
