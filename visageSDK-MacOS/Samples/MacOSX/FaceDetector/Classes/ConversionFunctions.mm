//
//  HelperFunctions.m
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "ConversionFunctions.h"

@implementation HelperFunctions

@end

// helper function for converting NSImage to IplImage
IplImage *IplImageFromNSImage(NSImage *image)
{
    //converting the NSImage into an IplImage
    NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
    
    int depth       = (int)[bitmap bitsPerSample];
    int channels    = (int)[bitmap samplesPerPixel];
    int height      = (int)[bitmap pixelsHigh];
    int width       = (int)[bitmap pixelsWide];
    
    IplImage *temp = cvCreateImageHeader(cvSize(width,height), depth, channels);
    cvSetImageData(temp, [bitmap bitmapData], (int)[bitmap bytesPerRow]);
    
    IplImage *iplpic = cvCloneImage(temp);
    
    cvReleaseImageHeader(&temp);
    
    return iplpic;
};

// helper function for creating an NSImage from an IplImage
NSImage* NSImageFromIplImage(IplImage* iplImage)
{
    BOOL hasAlpha = iplImage->nChannels == 4? YES:NO;
    
    NSBitmapImageRep *bmp= [[NSBitmapImageRep alloc]
                            initWithBitmapDataPlanes:0
                            pixelsWide:iplImage->width
                            pixelsHigh:iplImage->height
                            bitsPerSample:iplImage->depth
                            samplesPerPixel:iplImage->nChannels
                            hasAlpha:hasAlpha isPlanar:NO
                            colorSpaceName:NSDeviceRGBColorSpace
                            bytesPerRow:iplImage->widthStep
                            bitsPerPixel:0];
    if(hasAlpha)
    {
        NSUInteger val[4]= {0, 0, 0, 0};
        for(int x=0; x < iplImage->width; x++) {
            for(int y=0; y < iplImage->height; y++) {
                CvScalar scal= cvGet2D(iplImage, y, x);
                val[0]= scal.val[0];
                val[1]= scal.val[1];
                val[2]= scal.val[2];
                val[3]= scal.val[3];
                [bmp setPixel:val atX:x y:y];
            }
        }
    }
    else
    {
        NSUInteger val[3]= {0, 0, 0};
        for(int x=0; x < iplImage->width; x++) {
            for(int y=0; y < iplImage->height; y++) {
                CvScalar scal= cvGet2D(iplImage, y, x);
                val[0]= scal.val[0];
                val[1]= scal.val[1];
                val[2]= scal.val[2];
                [bmp setPixel:val atX:x y:y];
            }
        }
    }
    
    NSImage *im= [[NSImage alloc] initWithData:[bmp TIFFRepresentation]];
    return im;
};

