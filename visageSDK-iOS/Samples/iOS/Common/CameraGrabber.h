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

@interface CameraGrabber : UIViewController <AVCaptureVideoDataOutputSampleBufferDelegate>

{
	AVCaptureSession *_captureSession;
	AVCaptureConnection *_videoConnection;
	const NSString *_preset;
	UIImageView *_imageView;
	AVCaptureVideoPreviewLayer *_prevLayer;

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
}

/*!
 @brief	The capture session takes the input from the camera and capture it
 */
@property (nonatomic, retain) AVCaptureSession *captureSession;

@property (nonatomic, retain) AVCaptureConnection *videoConnection;
@property (strong, nonatomic) AVCaptureDevice     *defaultDevice;

/**
 * @abstract Accessing to the `AVCaptureMetadataOutput` object.
 * @discussion It allows you to configure the scanner to restrict the area of
 * the scan to the overlay one for example.
 * @since 3.5.0
 */
@property (readonly) AVCaptureMetadataOutput * _Nonnull metadataOutput;

@property (nonatomic, retain) const NSString *preset;
/*!
 @brief	The UIImageView we use to display the image generated from the imageBuffer
 */
@property (nonatomic, retain) UIImageView *imageView;
/*!
 @brief	The CALAyer customized by apple to display the video corresponding to a capture session
 */
@property (nonatomic, retain) AVCaptureVideoPreviewLayer *prevLayer;

@property (nonatomic) int wb;
@property (nonatomic) int rb;
@property (nonatomic) int ub;
@property (nonatomic) BOOL newFrame;
@property (copy, nonatomic) void (^completionBlock) (NSString *);

- (id)initWithSessionPreset:(const NSString*)preset UseFPS:(int)fps withDevice:(int)device;
/*!
 @brief	This method initializes the capture session
 */
- (void)initCapture;
- (void)startCapture:(UIView *) camView;
- (void)stopCapture;
- (int)getPixelFormat;
//- (void)getFrame:(unsigned char*) imageData;
- (unsigned char*)getBuffer:(int)rotated isMirrored:(int)mirrored;

-(void) toggleTorch;


#pragma mark - Managing the Orientation
/** @name Managing the Orientation */

/**
 * @abstract Returns the video orientation correspongind to the given interface
 * orientation.
 * @param interfaceOrientation An interface orientation.
 * @return the video orientation correspongind to the given device orientation.
 * @since 3.1.0
 */
+ (AVCaptureVideoOrientation)videoOrientationFromInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation;

#pragma mark - Managing the Block
/** @name Managing the Block */

/**
 * @abstract Sets the completion with a block that executes when a QRCode
 * or when the user did stopped the scan.
 * @param completionBlock The block to be executed. This block has no
 * return value and takes one argument: the `resultAsString`. If the user
 * stop the scan and that there is no response the `resultAsString` argument
 * is nil.
 * @since 3.0.0
 */
- (void)setCompletionWithBlock:(nullable void (^) (NSString * _Nullable resultAsString))completionBlock;

@end


@interface CameraGrabber() <AVCaptureMetadataOutputObjectsDelegate>

- (void)initScanner;
/**
 * @abstract Starts scanning the codes.
 * @since 3.0.0
 */
- (void)startScanning;

/**
 * @abstract Stops scanning the codes.
 * @since 3.0.0
 */
- (void)stopScanning;

/**
 * @abstract Indicates whether the session is currently running.
 * @discussion The value of this property is a Bool indicating whether the
 * receiver is running.
 * Clients can key value observe the value of this property to be notified
 * when the session automatically starts or stops running.
 * @since 3.3.0
 */
- (BOOL)running;


/**
 * @abstract Toggles torch on the default device.
 * @since 4.0.0
 */
- (void)toggleTorch;

@end
