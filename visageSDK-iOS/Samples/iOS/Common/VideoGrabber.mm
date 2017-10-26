#import "VideoGrabber.h"

#define FRAME_BGRA 1

@implementation VideoGrabber

@synthesize reader;
@synthesize readerVideoTrackOutput;
@synthesize asset;

@synthesize outputWidth, outputHeight;
@synthesize width, height;
@synthesize framerate;
@synthesize rotated;
@synthesize buffer;
@synthesize bufferTmp;
@synthesize areBuffersInited;

//+ (UIInterfaceOrientation)orientationForTrack:(AVAsset *)asset
+ (int)orientationForTrack:(AVAsset *)asset
{
    AVAssetTrack *videoTrack = [[asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
    CGSize size = [videoTrack naturalSize];
    CGAffineTransform txf = [videoTrack preferredTransform];
    
    if (size.width == txf.tx && size.height == txf.ty) {
        //NSLog(@"LandscapeRight");
        return 0;//UIInterfaceOrientationLandscapeRight;
    }
    else if (txf.tx == 0 && txf.ty == 0) {
        //NSLog(@"LandscapeLeft");
        return 1;//UIInterfaceOrientationLandscapeLeft;
    }
    else if (txf.tx == 0 && txf.ty == size.width) {
        //NSLog(@"PortraitUpsideDown");
        return 2;//UIInterfaceOrientationPortraitUpsideDown;
    }
    else {
        //NSLog(@"Portrait");
        return 3;//UIInterfaceOrientationPortrait;
    }
}

- (int) readMovie:(NSURL *)url
{
    areBuffersInited = NO;
	asset = [AVURLAsset URLAssetWithURL:url options:nil];
    /*
	[asset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:@"tracks"] completionHandler:
	 ^{
		 dispatch_async(dispatch_get_main_queue(),
			^{
				AVAssetTrack * videoTrack = nil;

				NSArray * tracks = [asset tracksWithMediaType:AVMediaTypeVideo];
				if ([tracks count] == 1)
				{
					videoTrack = [tracks objectAtIndex:0];

					framerate = videoTrack.nominalFrameRate;
                    width = videoTrack.naturalSize.width;
                    height = videoTrack.naturalSize.height;
                    
                    NSLog(@"Video is %dx%d", width, height);

					NSError * error = nil;

					// reader is a member variable
					self.reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
					if (error)
						NSLog(@"%@", error.localizedDescription);		

					//NSString* key = (NSString*)kCVPixelBufferPixelFormatTypeKey;
					//NSNumber* value = [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA];
					NSDictionary* videoSettings;
					//[NSDictionary dictionaryWithObject:value forKey:key];

					if (outputWidth && outputHeight) {	
						videoSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
						 [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], (NSString*)kCVPixelBufferPixelFormatTypeKey,
						 [NSNumber numberWithUnsignedInt:outputWidth], (NSString*)kCVPixelBufferWidthKey,
						 [NSNumber numberWithUnsignedInt:outputHeight], (NSString*)kCVPixelBufferHeightKey,
						 nil];
					} else {
#if FRAME_BGRA
						
						videoSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
										 [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], (NSString*)kCVPixelBufferPixelFormatTypeKey,
										 nil];
#else
						videoSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
										 [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8Planar], (NSString*)kCVPixelBufferPixelFormatTypeKey,
										 nil];
#endif
					}
					[reader addOutput:[AVAssetReaderTrackOutput 
						   assetReaderTrackOutputWithTrack:videoTrack 
						   outputSettings:videoSettings]];
					[reader startReading];
					[videoSettings autorelease];
				}
			});
	 }];
     */
    
    AVAssetTrack* videoTrack = nil;
    NSArray* tracks = [asset tracksWithMediaType:AVMediaTypeVideo];
    if ([tracks count] == 1)
    {
        videoTrack = [tracks objectAtIndex:0];
        width = videoTrack.naturalSize.width;
        height = videoTrack.naturalSize.height;
        framerate = videoTrack.nominalFrameRate;

        NSError* error = nil;
        reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
        if (error)
            NSLog(@"%@", [error localizedDescription]);             

        NSDictionary* videoSettings = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA]
                                                                  forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];

        [reader addOutput:[AVAssetReaderTrackOutput
                                 assetReaderTrackOutputWithTrack:videoTrack
                                 outputSettings:videoSettings]];
        [reader startReading];
    }
    
    return [VideoGrabber orientationForTrack:asset];
}

-(int)startGrabbing:(NSString *)filename {	
	framerate = 30;
	buffer = nil;
    //NSLog(@"Video is %@", filename);
	//rotated = [self readMovie:[NSURL fileURLWithPath:[Utilities bundlePath:filename]]];
    
    rotated = [self readMovie:[NSURL fileURLWithPath:filename]];
    
	return rotated;
}

-(BOOL)isGrabbing {
	if (reader.status == AVAssetReaderStatusReading) {
		return YES;
    }
	else {
		return NO;
	}
}

- (UIImage*) readNextMovieFrame
{
	if (reader.status == AVAssetReaderStatusReading)
	{
		AVAssetReaderOutput * output = [reader.outputs objectAtIndex:0];
		CMSampleBufferRef sampleBuffer = [output copyNextSampleBuffer];
		if (sampleBuffer)
		{
			CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer); 
			
			// Lock the image buffer
			CVPixelBufferLockBaseAddress(imageBuffer,0); 
			
			// Get information of the image
			uint8_t *baseAddress = (uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
			//uint8_t *yBase = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
			size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
			size_t width_ = CVPixelBufferGetWidth(imageBuffer);
			size_t height_ = CVPixelBufferGetHeight(imageBuffer);
			
			/*Create a CGImageRef from the CVImageBufferRef*/
			CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB(); 
			CGContextRef newContext = CGBitmapContextCreate(baseAddress, width_, height_, 8, bytesPerRow, colorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
			CGImageRef newImage = CGBitmapContextCreateImage(newContext); 
			
			/*We release some components*/
			CGContextRelease(newContext); 
			CGColorSpaceRelease(colorSpace);
			
			/*We display the result on the image view (We need to change the orientation of the image so that the video is displayed correctly).
			 Same thing as for the CALayer we are not in the main thread so ...*/
			UIImage *image = [UIImage imageWithCGImage:newImage scale:1.0 orientation:UIImageOrientationUp];
			
			/*We relase the CGImageRef*/
			CGImageRelease(newImage);
			
			// Unlock the image buffer
			CVPixelBufferUnlockBaseAddress(imageBuffer,0);
			CFRelease(sampleBuffer);
			
			return image;
		}
	}
	return nil;
}
- (VsImage*) getNextMovieFrame:(BOOL)skip
{
	if (reader.status == AVAssetReaderStatusReading)
	{
		AVAssetReaderOutput * output = [reader.outputs objectAtIndex:0];
		CMSampleBufferRef sampleBuffer = [output copyNextSampleBuffer];
		if (sampleBuffer && !skip)
		{
			CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer); 
			
			// Lock the image buffer
			CVPixelBufferLockBaseAddress(imageBuffer,0); 
			
			// Get information of the image
			uint8_t *baseAddress = (uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
			//uint8_t *yBaseAddress = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
			//size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
			int width_ = (int)CVPixelBufferGetWidth(imageBuffer);
			int height_ = (int)CVPixelBufferGetHeight(imageBuffer);
            
            uint8_t *src;
#if FRAME_BGRA
            src = baseAddress;
            if (!areBuffersInited) {
                bufferTmp = vsCreateImage(vsSize(width_, height_), VS_DEPTH_8U, 4);
                if (rotated == 2 || rotated == 3) {
                    buffer = vsCreateImage(vsSize(height_, width_), VS_DEPTH_8U, 4);
                } else {
                    buffer = vsCreateImage(vsSize(width_, height_), VS_DEPTH_8U, 4);
                }
                areBuffersInited = YES;
            }
			//if (buffer == nil)
			//	buffer = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);
			//memcpy(buffer->imageData, baseAddress, buffer->imageSize);
#else
            src = yBaseAddress;
            if (!areBuffersInited) {
                bufferTmp = vsCreateImage(vsSize(width_, height_), VS_DEPTH_8U, 1);
                if (rotated == 2 || rotated == 3) {
                    buffer = vsCreateImage(vsSize(height_, width_), VS_DEPTH_8U, 1);
                } else {
                    buffer = vsCreateImage(vsSize(width_, height_), VS_DEPTH_8U, 1);
                }
                areBuffersInited = YES;
            }
			//if (buffer == nil)
			//	buffer = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
			//memcpy(buffer->imageData, yBaseAddress, buffer->imageSize);
#endif
            char *ptrImageData;

            switch (rotated) {
                case 0:
                    // no rotation
                    ptrImageData = bufferTmp->imageData;
                    bufferTmp->imageData = (char *) src;
                    
                    // rotating the frame
                    vsFlip(bufferTmp, buffer,-1);
                    bufferTmp->imageData = ptrImageData;
                    break;
                case 1:
                    // no rotation
                    memcpy(buffer->imageData, src, buffer->imageSize);
                    break;
                case 2:
                    ptrImageData = bufferTmp->imageData;
                    bufferTmp->imageData = (char *) src;
                    
                    // rotating the frame
                    vsTranspose(bufferTmp, buffer);
                    bufferTmp->imageData = ptrImageData;
                    vsFlip(buffer,buffer,0);
                    break;
                case 3:
                    ptrImageData = bufferTmp->imageData;
                    bufferTmp->imageData = (char *) src;
                    
                    // rotating the frame
                    vsTranspose(bufferTmp, buffer);
                    vsFlip(buffer,buffer,1);
                    bufferTmp->imageData = ptrImageData;
                    break;
            }
            /*
            if (rotated == 1) {
                char *ptrImageData = bufferTmp->imageData;
                bufferTmp->imageData = (char *) src;

                // rotating the frame
                cvTranspose(bufferTmp, buffer);
                bufferTmp->imageData = ptrImageData;
            } else {
                // no rotation
                memcpy(buffer->imageData, src, buffer->imageSize);
            }
             */
			// Unlock the image buffer
			CVPixelBufferUnlockBaseAddress(imageBuffer,0);
			CFRelease(sampleBuffer);
			
			return buffer;
		}
        if(sampleBuffer)
            CFRelease(sampleBuffer);
	}
	return nil;
}

- (void)dealloc
{
#if DEBUG
    reader = nil;
    
    if (areBuffersInited) {
        vsReleaseImage(&bufferTmp);
        vsReleaseImage(&buffer);
    }
    
    [super dealloc];
#else
    reader = nil;
    
    if (areBuffersInited) {
        vsReleaseImage(&bufferTmp);
        vsReleaseImage(&buffer);
    }
    
#endif
}


@end
