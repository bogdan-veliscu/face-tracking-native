#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>
#import <visageVision.h>

// declare C++ implementation for .m (Obj-C) files
#ifdef __OBJC__
#ifndef __cplusplus
typedef void VsImage;
#endif
#endif

@interface VideoGrabber : NSObject {
	AVAssetReader* reader;
	AVAssetReaderTrackOutput* readerVideoTrackOutput;
    AVURLAsset* asset;
	
	int outputWidth, outputHeight;
    int width, height;
	float framerate;
    int rotated;
	VsImage *buffer;
    VsImage *bufferTmp;
    BOOL areBuffersInited;
}

@property (nonatomic) AVAssetReader* reader;
@property (nonatomic) AVAssetReaderTrackOutput* readerVideoTrackOutput;
@property (nonatomic) AVURLAsset* asset;

/* Output image size. Set to the source size by default. */
@property (nonatomic) int outputWidth, outputHeight;
@property (nonatomic) int width, height;
@property (nonatomic) float framerate;
@property (nonatomic) int rotated;
@property (nonatomic) VsImage *buffer;
@property (nonatomic) VsImage *bufferTmp;
@property (nonatomic) BOOL areBuffersInited;

-(int)startGrabbing:(NSString *)filename;
-(BOOL)isGrabbing;
-(UIImage*)readNextMovieFrame;
-(VsImage*)getNextMovieFrame:(BOOL)skip;
-(void)dealloc;

@end
