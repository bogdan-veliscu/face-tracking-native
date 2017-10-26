#import <Foundation/Foundation.h>

// declare C++ implementation for .mm (Obj-C++) files
#ifdef __cplusplus
#include "VisageFaceRecognition.h"
#include "VisageFeaturesDetector.h"
#endif

// declare C++ implementation for .m (Obj-C) files
#ifdef __OBJC__
#ifndef __cplusplus
typedef void VisageFaceRecognition;
#endif
#endif

/**
 * Implements simple high-level Objective-C interface around VisageSDK::VisageFaceRecognition functionallity.
 *
 * This makes connecting the iOS app GUI with visage|SDK features recognizer functionaly easier as primary programming language used in iOS apps is
 * Objective-C while primary programming language used in visage|SDK is C++. The class is a subclass of NSObject and
 * it uses Objective-C++ for implementation details.
 */
@interface VisageFaceRecognitionWrapper : NSObject
{
#ifdef __cplusplus
     VisageSDK::VisageFaceRecognition* vfr;
     VisageSDK::VisageFeaturesDetector* vfd;
#else
     VisageFaceRecognition* vfr;
     VisageFeaturesDetector* vfd;
#endif
   
     //logo image
     @public IplImage* logo;
}

/**
 * Method for initializing the recognizer.
 *
 * This method creates a new VisageFaceRecognition object and calls method createGallery for loading VisageFaceRecognition gallery.
 * Returns false if VisageFaceRecognition object cannot be initialized or if the gallery cannot be created.
 */
#ifdef __cplusplus
- (bool) initRecognition:(VisageSDK::VisageFeaturesDetector*) detector;
#else
- (bool) initRecognition:(VisageFeaturesDetector*) detector;
#endif

/**
 * Method which adds descriptors to VisageFaceRecognition gallery.
 *
 * This method goes through all pre-loaded images in the Famous Actors/gallery/ folder and adds corresponding descriptor to VisageFaceRecognition gallery for each.
 * Returns 0 if VisageFeaturesDetector object has not been initialized yet or if the Famous Actors/gallery/ folder is empty.
 */
- (int) createGallery;

/**
 * Method which performs face recognition on a given image.
 *
 * Performs face recognition on a given image and returns the image with the name of the recognized person or the person with the most similar face found in the VisageFaceRecognition gallery.
 */
- (UIImage*) recognizeFace:(UIImage*) uiImage;

@end
