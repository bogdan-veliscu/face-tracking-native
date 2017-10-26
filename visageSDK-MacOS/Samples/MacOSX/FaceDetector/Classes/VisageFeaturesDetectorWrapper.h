//
//  VisageFeaturesDetectorWrapper.h
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ConversionFunctions.h"

#ifdef __cplusplus
#include "VisageFeaturesDetector.h"

namespace VisageSDK
{
    void initializeLicenseManager(const char *licenseKeyFileFolder);
}
#else
typedef void VisageFeaturesDetector;
#endif

/**
 * Implements simple high-level Objective-C interface around VisageSDK::VisageFeaturesDetector functionallity.
 *
 * This makes connecting the OS X app GUI with visage|SDK features detector functionaly easier as primary programming language used in OS X apps is
 * Objective-C while primary programming language used in visage|SDK is C++. The class is a subclass of NSObject and
 * it uses Objective-C++ for implementation details.
 */
@interface VisageFeaturesDetectorWrapper : NSObject
{
#ifdef __cplusplus
    VisageSDK::VisageFeaturesDetector* visageFeaturesDetector;
#else
    VisageFeaturesDetector* visageFeaturesDetector;
#endif
}

/**
 * Method for initializing the detector.
 *
 * This method creates a new VisageFeaturesDetector object tracking by choosing appropriate configuraion file based on device it is running on.
 */
- (void)initDetector;

/**
 * Method for drawing features on an image.
 *
 * This method detects the features on the input image and returns a new image with the features drawn.
 */
- (NSImage*)drawFeatures:(NSImage*)image;

@end
