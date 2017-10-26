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
#include "WrapperOpenCV.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

#import <pthread.h>

//#include "cv.h"


@interface CameraGrabber : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
	AVCaptureSession *_captureSession;
	AVCaptureConnection *_videoConnection;
    uint8_t *_buffers[3];
    uint8_t *_bufferR90;
	int _wb;
	int _rb;
	int _ub;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	BOOL areBuffersInited;
	BOOL _newFrame;
	int _fps;
    int _device;
    
    int frameWidth;
    int frameHeight;
    
    OSType pixelFormatType;
}

/*!
 @brief	The capture session takes the input from the camera and capture it
 */
@property (nonatomic, retain) AVCaptureSession *captureSession;

@property (nonatomic, retain) AVCaptureConnection *videoConnection;

@property (nonatomic) int wb;
@property (nonatomic) int rb;
@property (nonatomic) int ub;
@property (nonatomic) BOOL newFrame;

@property (nonatomic) int frameWidth;
@property (nonatomic) int frameHeight;

@property (nonatomic) OSType pixelFormatType;

- (id)initWithWidth:(int)width andHeight:(int)height UseFPS:(int)fps withDevice:(int)device;
/*!
 @brief	This method initializes the capture session
 */
- (void)initCapture;
- (void)startCapture;
- (void)stopCapture;
- (int)getPixelFormat;
- (unsigned char*)getBuffer:(int)rotated isMirrored:(int)mirrored;

@end