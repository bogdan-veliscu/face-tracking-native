//
//  HelperFunctions.h
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "cv.h"

@interface HelperFunctions : NSObject

/**
 * Function which converts  NSImage to IplImage used by OpenCV.
 * @param image NSImage
 * @return IplImage representation
 */
IplImage* IplImageFromUIImage(UIImage* image, int nChannels);
/**
 * Function which converts  IplImage to NSImage used by OS X.
 * @param image Iplmage
 * @return NSImage representation
 */
UIImage* UIImageFromIplImage(IplImage* iplImage);

@end

