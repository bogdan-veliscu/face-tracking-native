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

#import "CameraGrabber.h"

#import <mach/mach_time.h>
#import <sys/time.h>

#define PREVLAYER 0


@implementation NSNumber (CVPixelFormatType)

- (NSString *)descriptivePixelFormat
{
    return @{
             @(kCVPixelFormatType_1Monochrome): @"kCVPixelFormatType_1Monochrome",
             @(kCVPixelFormatType_2Indexed): @"kCVPixelFormatType_2Indexed",
             @(kCVPixelFormatType_4Indexed): @"kCVPixelFormatType_4Indexed",
             @(kCVPixelFormatType_8Indexed): @"kCVPixelFormatType_8Indexed",
             @(kCVPixelFormatType_1IndexedGray_WhiteIsZero): @"kCVPixelFormatType_1IndexedGray_WhiteIsZero",
             @(kCVPixelFormatType_2IndexedGray_WhiteIsZero): @"kCVPixelFormatType_2IndexedGray_WhiteIsZero",
             @(kCVPixelFormatType_4IndexedGray_WhiteIsZero): @"kCVPixelFormatType_4IndexedGray_WhiteIsZero",
             @(kCVPixelFormatType_8IndexedGray_WhiteIsZero): @"kCVPixelFormatType_8IndexedGray_WhiteIsZero",
             @(kCVPixelFormatType_16BE555): @"kCVPixelFormatType_16BE555",
             @(kCVPixelFormatType_16LE555): @"kCVPixelFormatType_16LE555",
             @(kCVPixelFormatType_16LE5551): @"kCVPixelFormatType_16LE5551",
             @(kCVPixelFormatType_16BE565): @"kCVPixelFormatType_16BE565",
             @(kCVPixelFormatType_16LE565): @"kCVPixelFormatType_16LE565",
             @(kCVPixelFormatType_24RGB): @"kCVPixelFormatType_24RGB",
             @(kCVPixelFormatType_24BGR): @"kCVPixelFormatType_24BGR",
             @(kCVPixelFormatType_32ARGB): @"kCVPixelFormatType_32ARGB",
             @(kCVPixelFormatType_32BGRA): @"kCVPixelFormatType_32BGRA",
             @(kCVPixelFormatType_32ABGR): @"kCVPixelFormatType_32ABGR",
             @(kCVPixelFormatType_32RGBA): @"kCVPixelFormatType_32RGBA",
             @(kCVPixelFormatType_64ARGB): @"kCVPixelFormatType_64ARGB",
             @(kCVPixelFormatType_48RGB): @"kCVPixelFormatType_48RGB",
             @(kCVPixelFormatType_32AlphaGray): @"kCVPixelFormatType_32AlphaGray",
             @(kCVPixelFormatType_16Gray): @"kCVPixelFormatType_16Gray",
             @(kCVPixelFormatType_422YpCbCr8): @"kCVPixelFormatType_422YpCbCr8",
             @(kCVPixelFormatType_4444YpCbCrA8): @"kCVPixelFormatType_4444YpCbCrA8",
             @(kCVPixelFormatType_4444YpCbCrA8R): @"kCVPixelFormatType_4444YpCbCrA8R",
             @(kCVPixelFormatType_444YpCbCr8): @"kCVPixelFormatType_444YpCbCr8",
             @(kCVPixelFormatType_422YpCbCr16): @"kCVPixelFormatType_422YpCbCr16",
             @(kCVPixelFormatType_422YpCbCr10): @"kCVPixelFormatType_422YpCbCr10",
             @(kCVPixelFormatType_444YpCbCr10): @"kCVPixelFormatType_444YpCbCr10",
             @(kCVPixelFormatType_420YpCbCr8Planar): @"kCVPixelFormatType_420YpCbCr8Planar",
             @(kCVPixelFormatType_420YpCbCr8PlanarFullRange): @"kCVPixelFormatType_420YpCbCr8PlanarFullRange",
             @(kCVPixelFormatType_422YpCbCr_4A_8BiPlanar): @"kCVPixelFormatType_422YpCbCr_4A_8BiPlanar",
             @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange): @"kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange",
             @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange): @"kCVPixelFormatType_420YpCbCr8BiPlanarFullRange",
             @(kCVPixelFormatType_422YpCbCr8_yuvs): @"kCVPixelFormatType_422YpCbCr8_yuvs",
             @(kCVPixelFormatType_422YpCbCr8FullRange): @"kCVPixelFormatType_422YpCbCr8FullRange"
             }[self];
}

@end


@implementation CameraGrabber

@synthesize captureSession = _captureSession;
@synthesize videoConnection = _videoConnection;
@synthesize wb = _wb;
@synthesize rb = _rb;
@synthesize ub = _ub;
@synthesize newFrame = _newFrame;

@synthesize frameWidth;
@synthesize frameHeight;

@synthesize pixelFormatType;

int width;
int height;

#pragma mark -
#pragma mark Initialization
- (id)init
{
	if (self) {
		/*We initialize some variables (they might be not initialized depending on what is commented or not)*/
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&cond, NULL);
		areBuffersInited = NO;
		self.wb = 0;
		self.rb = 1;
		self.ub = 2;
		self.newFrame = NO;
		_buffers[0] = nil;
		_buffers[1] = nil;
		_buffers[2] = nil;
		_bufferR90 = nil;
		_fps = 30;
        _device = 0;
        
        self.frameWidth = 640;
        self.frameHeight = 480;
        
        self.pixelFormatType = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
	}

	return self;
}

- (id)initWithWidth:(int)width andHeight:(int)height UseFPS:(int)fps withDevice:(int)device
{
    if (self)
    {
        /*We initialize some variables (they might be not initialized depending on what is commented or not)*/
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        areBuffersInited = NO;
        self.wb = 0;
        self.rb = 1;
        self.ub = 2;
        self.newFrame = NO;
        _buffers[0] = nil;
        _buffers[1] = nil;
        _buffers[2] = nil;
        _bufferR90 = nil;
        _fps = fps;
        _device = device;
        
        self.frameWidth = width;
        self.frameHeight = height;
        
        self.pixelFormatType = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
    }
    
    return self;
}

- (void)initCapture
{
	/*We setup the input*/
	AVCaptureDeviceInput *captureInput = nil;
	NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];//[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    if ([devices count] > _device)
        captureInput = [AVCaptureDeviceInput deviceInputWithDevice:[devices objectAtIndex:_device] error:nil];
    else
        captureInput = [AVCaptureDeviceInput deviceInputWithDevice:[devices objectAtIndex:0] error:nil];
	
	if (captureInput == nil) {
		NSLog(@"No camera! Exiting...");
		return;
	}
	
	/*We setupt the output*/
	AVCaptureVideoDataOutput *captureOutput = [[AVCaptureVideoDataOutput alloc] init];
	/*While a frame is processes in -captureOutput:didOutputSampleBuffer:fromConnection: delegate methods no other frames are added in the queue.
	 If you don't want this behaviour set the property to NO */
	captureOutput.alwaysDiscardsLateVideoFrames = YES;
	
	/*We create a serial queue to handle the processing of our frames*/
	dispatch_queue_t captureQueue;
	captureQueue = dispatch_queue_create("cameraQueue", NULL);
	[captureOutput setSampleBufferDelegate:self queue:captureQueue];
    
    const NSArray *supportedTypes = captureOutput.availableVideoCVPixelFormatTypes;
    
    NSMutableArray *mutablePixelFormatTypes = [NSMutableArray array];
    [supportedTypes enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        [mutablePixelFormatTypes addObject:[obj descriptivePixelFormat]];
    }];
    NSString *pixelFormats = [mutablePixelFormatTypes componentsJoinedByString:@",\n"];
    NSLog(@"Available pixel formats:\n%@\n", pixelFormats);
    
  
    for (NSNumber *currentPixelFormat in supportedTypes) {
        
        if ([currentPixelFormat intValue] == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {
            pixelFormatType = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
            break;
        } else if ([currentPixelFormat intValue] == kCVPixelFormatType_32BGRA) {
            pixelFormatType = kCVPixelFormatType_32BGRA;
            break;
        }
    }
    
    
    NSDictionary *pixelBufferOptions = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithDouble:frameWidth], (id)kCVPixelBufferWidthKey,
                                        [NSNumber numberWithDouble:frameHeight], (id)kCVPixelBufferHeightKey,
                                        [NSNumber numberWithUnsignedInt:pixelFormatType], (id)kCVPixelBufferPixelFormatTypeKey,
                                        nil];
    [captureOutput setVideoSettings:pixelBufferOptions];
    
	/*And we create a capture session*/
	self.captureSession = [[AVCaptureSession alloc] init];
    
	/*We add input and output*/
	[self.captureSession addInput:captureInput];
	[self.captureSession addOutput:captureOutput];

    AVCaptureConnection *conn = [captureOutput connectionWithMediaType:AVMediaTypeVideo];
    if ([conn isVideoMinFrameDurationSupported]){
        [conn setVideoMinFrameDuration:CMTimeMake(1, _fps)];
    }


}

#pragma mark -
#pragma mark AVCaptureSession delegate
- (void)captureOutput:(AVCaptureOutput *)captureOutput 
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer 
	   fromConnection:(AVCaptureConnection *)connection 
{
	//We create an autorelease pool because as we are not in the main_queue our code is
	//not executed in the main thread. So we have to create an autorelease pool for the thread we are in
	//NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    //Lock the image buffer
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    //Get information about the image
    uint8_t *baseAddress = (uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
	uint8_t *yBaseAddress = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    //size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    width = (int)CVPixelBufferGetWidth(imageBuffer);
    height = (int)CVPixelBufferGetHeight(imageBuffer);
    
    frameWidth = width;
    frameHeight = height;
    
    unsigned char *src = 0;
    
    if (pixelFormatType == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {
        int size = (width*height)*3/2;
        if (!areBuffersInited) {
        
            _bufferR90 = malloc(sizeof(uint8_t)*size);
        
            for (int i = 0; i < 3; i++)
                _buffers[i] = malloc(sizeof(uint8_t)*size);
        
            areBuffersInited = YES;
        }
    
        src = (unsigned char*) yBaseAddress;
        // using memcpy
        memcpy(_buffers[self.wb], (char *)src, sizeof(uint8_t)*size);
        
    } else {
        int size = (width*height)*4;
        if (!areBuffersInited) {
            _bufferR90 = malloc(sizeof(uint8_t)*size);
            for (int i = 0; i < 3; i++) {
                _buffers[i] = malloc(sizeof(uint8_t)*size);
            }
            areBuffersInited = YES;
        }
	
        src = (unsigned char*) baseAddress;
        // using memcpy
        memcpy(_buffers[self.wb], (char *)src, sizeof(uint8_t)*size);
    }
    
	pthread_mutex_lock(&mutex);
	int tmp = self.wb;
	self.wb = self.rb;
	self.rb = tmp;
	self.newFrame = YES;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	//We unlock the  image buffer
	CVPixelBufferUnlockBaseAddress(imageBuffer,0);
	
	//[pool drain];
}

-(void) startCapture
{
	[self initCapture];
	[self.captureSession startRunning];

    //
    pthread_mutex_lock(&mutex);

    while (!self.newFrame)
    {
        int ret = pthread_cond_wait(&cond, &mutex);
        if (ret != 0)
        {
            NSLog(@"startCapture:Error while waiting on mutex condition!");
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    pthread_mutex_unlock(&mutex);


}

- (void)stopCapture
{
	[self.captureSession stopRunning];
	pthread_mutex_lock(&mutex);
	NSLog(@"Stopping camera capture...");
	self.newFrame = YES;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

- (int)getPixelFormat
{
    if (pixelFormatType == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
        return 1;
    else
        return 4;
}

- (unsigned char*)getBuffer:(int)rotated isMirrored:(int)mirrored
{
    struct timespec   ts;
    struct timeval    tp;
    
    /* Convert from timeval to timespec */
    ts.tv_sec  = tp.tv_sec + 5;
    ts.tv_nsec = 0;

	pthread_mutex_lock(&mutex);
	
	while (!self.newFrame) {
		//int ret = pthread_cond_timedwait(&cond, &mutex, &ts);
        int ret = pthread_cond_wait(&cond, &mutex);
        if (ret != 0)
        {
            NSLog(@"Error while waiting on mutex condition!");
            pthread_mutex_unlock(&mutex);
            return 0;
        }
	}
	
	int tmp = self.ub;
	self.ub = self.rb;
	self.rb = tmp;
	self.newFrame = NO;
	pthread_mutex_unlock(&mutex);
	
    
    if (_buffers[self.ub] != nil) {
        if (pixelFormatType == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
            [self rotateYUV: _buffers[self.ub] _saveTo:_bufferR90 _width:width _height:height _rotation:rotated _mirrored:mirrored];
        else
            [self rotateBGRA: _buffers[self.ub] _saveTo:_bufferR90 _width:width _height:height _rotation:rotated _mirrored:mirrored];
    }
    
    return (unsigned char*)_bufferR90;
   
}


- (void)rotateYUV:(uint8_t*)yuv _saveTo:(uint8_t*)output _width:(int)width _height:(int)height _rotation:(int)rotation _mirrored:(int)mirrored
{
    int frameSize = width * height;
    Boolean swap = false;
    Boolean xflip = false;
    Boolean yflip = false;
    
    if (rotation == 1)
        swap = true;
    
    if (mirrored == 1)
        xflip = true;
  
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int yIn = j * width + i;
            int uIn = frameSize + (j >> 1) * width + (i & ~1);
            int vIn = uIn       + 1;
            
            int wOut     = swap  ? height              : width;
            int hOut     = swap  ? width               : height;
            int iSwapped = swap  ? j                   : i;
            int jSwapped = swap  ? i                   : j;
            int iOut     = xflip ? wOut - iSwapped - 1 : iSwapped;
            int jOut     = yflip ? hOut - jSwapped - 1 : jSwapped;
            
            int yOut = jOut * wOut + iOut;
            int uOut = frameSize + (jOut >> 1) * wOut + (iOut & ~1);
            int vOut = uOut + 1;
            
            output[yOut] = (0xff & yuv[yIn]);
            output[uOut] = (0xff & yuv[uIn]);
            output[vOut] = (0xff & yuv[vIn]);
        }
    }
}

- (void)rotateBGRA:(uint8_t*)rgba _saveTo:(uint8_t*)output _width:(int)width _height:(int)height _rotation:(int)rotation _mirrored: (int)mirrored
{
    Boolean swap = (rotation == 1);
    Boolean xflip = false;
    Boolean yflip = false;
    
    if (mirrored == 1)
        xflip = true;
  
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int rIn = j * width * 4 + 4*i + 0;
            int gIn = j * width * 4 + 4*i + 1;
            int bIn = j * width * 4 + 4*i + 2;
            int aIn = j * width * 4 + 4*i + 3;
            
            int wOut     = swap  ? height              : width;
            int hOut     = swap  ? width               : height;
            int iSwapped = swap  ? j                   : i;
            int jSwapped = swap  ? i                   : j;
            int iOut     = xflip ? wOut - iSwapped - 1 : iSwapped;
            int jOut     = yflip ? hOut - jSwapped - 1 : jSwapped;
            
            int rOut = jOut * wOut * 4 + 4*iOut + 0;
            int gOut = jOut * wOut * 4 + 4*iOut + 1;
            int bOut = jOut * wOut * 4 + 4*iOut + 2;
            int aOut = jOut * wOut * 4 + 4*iOut + 3;
            
            output[rOut] = (0xff & rgba[rIn]);
            output[gOut] = (0xff & rgba[gIn]);
            output[bOut] = (0xff & rgba[bIn]);
            output[aOut] = (0xff & rgba[aIn]);
        }
    }
}


#pragma mark -
#pragma mark Memory management
- (void)dealloc
{
	[self stopCapture];
	//[_captureSession release];

	if (areBuffersInited) {
		for (int i = 0; i < 3; i++) {
			free (_buffers[i]);
		}
		
		free (_bufferR90);
	}
    
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

    //[super dealloc];
}

@end
