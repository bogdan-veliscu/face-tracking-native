//
//  HelperFunctions.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/11/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "cv.h"
#include "visageVision.h"

@interface HelperFunctions : NSObject

@end

/**
 * Function which converts NSImage to VsImage.
 * @param image NSImage
 * @param nChannels number of channels
 */
VsImage *CreateVsImageFromNSImage(NSImage *image, int nChannels);

/**
 * Function which converts NSImage to IplImage.
 * @param image NSImage
 * @param nChannels number of channels
 */
IplImage *CreateIplImageFromNSImage(NSImage *image, int nChannels);

/**
 * Function that extracts CGI image representation from NSImage.
 * @param image NSImage
 * @return CGI image representation
 */
CGImageRef CGImageCreateWithNSImage(NSImage *image);

/**
 * Function which converts NSImage to IplImage used by OpenCV.
 * @param image NSImage
 * @return IplImage representation
 */
IplImage *IplImageFromNSImage(NSImage *image);