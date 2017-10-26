//
//  HelperFunctions.m
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/11/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "HelperFunctions.h"

@implementation HelperFunctions

@end

// helper function for converting NSImage to VsImage
VsImage *CreateVsImageFromNSImage(NSImage *image, int nChannels) {
    CGImageRef imageRef = CGImageCreateWithNSImage(image);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    VsImage *vsImage = vsCreateImage(vsSize(image.size.width, image.size.height), VS_DEPTH_8U, 4);
    CGContextRef contextRef = CGBitmapContextCreate(vsImage->imageData, vsImage->width, vsImage->height,
                                                    vsImage->depth, vsImage->widthStep,
                                                    colorSpace, kCGImageAlphaPremultipliedLast|kCGBitmapByteOrderDefault);
    CGContextDrawImage(contextRef, CGRectMake(0, 0, image.size.width, image.size.height), imageRef);
    CGContextRelease(contextRef);
    CGColorSpaceRelease(colorSpace);
    
    VsImage *ret = vsCreateImage(vsGetSize(vsImage), VS_DEPTH_8U, nChannels);
    switch (nChannels) {
        case 1:
            vsCvtColor(vsImage, ret, VS_RGBA2GRAY);
            break;
        case 4:
            vsCvtColor(vsImage, ret, VS_RGBA2BGRA);
            break;
        case 3:
            // fallthrough
        default:
            vsCvtColor(vsImage, ret, VS_RGBA2BGR);
            break;
    }
    
    vsReleaseImage(&vsImage);
    return ret;
}

// helper function for converting NSImage to IplImage
IplImage *CreateIplImageFromNSImage(NSImage *image, int nChannels) {
    CGImageRef imageRef = CGImageCreateWithNSImage(image);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    IplImage *iplimage = cvCreateImage(cvSize(image.size.width, image.size.height), IPL_DEPTH_8U, 4);
    CGContextRef contextRef = CGBitmapContextCreate(iplimage->imageData, iplimage->width, iplimage->height,
                                                    iplimage->depth, iplimage->widthStep,
                                                    colorSpace, kCGImageAlphaPremultipliedLast|kCGBitmapByteOrderDefault);
    CGContextDrawImage(contextRef, CGRectMake(0, 0, image.size.width, image.size.height), imageRef);
    CGContextRelease(contextRef);
    CGColorSpaceRelease(colorSpace);
    
    IplImage *ret = cvCreateImage(cvGetSize(iplimage), IPL_DEPTH_8U, nChannels);
    switch (nChannels) {
        case 1:
            cvCvtColor(iplimage, ret, CV_RGBA2GRAY);
            break;
        case 4:
            cvCvtColor(iplimage, ret, CV_RGBA2BGRA);
            break;
        case 3:
            // fallthrough
        default:
            cvCvtColor(iplimage, ret, CV_RGBA2BGR);
            break;
    }
    
    cvReleaseImage(&iplimage);
    return ret;
}

//helper function for getting CGImageRef form NSImage
CGImageRef CGImageCreateWithNSImage(NSImage *image)
{
    NSSize imageSize = [image size];
    
    CGContextRef bitmapContext = CGBitmapContextCreate(NULL, imageSize.width, imageSize.height, 8, 0, [[NSColorSpace genericRGBColorSpace] CGColorSpace], kCGBitmapByteOrder32Host|kCGImageAlphaPremultipliedFirst);
    
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithGraphicsPort:bitmapContext flipped:NO]];
    [image drawInRect:NSMakeRect(0, 0, imageSize.width, imageSize.height) fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
    [NSGraphicsContext restoreGraphicsState];
    
    CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
    CGContextRelease(bitmapContext);
    return cgImage;
}

// helper function for converting NSImage to IplImage
IplImage *IplImageFromNSImage(NSImage *image)
{
    //converting the NSImage into an IplImage
    NSBitmapImageRep *bitmap2 = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
    NSImage* bild1 = [[NSImage alloc] initWithSize:NSMakeSize([bitmap2 pixelsWide],[bitmap2 pixelsHigh])];
    
    NSInteger depth       = [bitmap2 bitsPerSample];
    NSInteger channels    = [bitmap2 samplesPerPixel];
    int height      = [bild1 size].height;
    int width       = [bild1 size].width;
    
    IplImage *iplpic = cvCreateImage(cvSize(width,height), (int)depth, (int)channels);
    unsigned char* bitmapData = [bitmap2 bitmapData];
    for (int i = 0; i < height; i++)
        memcpy(&iplpic->imageData[i * iplpic->widthStep], &bitmapData[i * width * channels], width * channels);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < iplpic->widthStep; j+= channels)
        {
            uchar tempR, tempG, tempB;
            tempR = iplpic->imageData[i * iplpic->widthStep + j];
            tempG = iplpic->imageData[i * iplpic->widthStep + j +1];
            tempB = iplpic->imageData[i * iplpic->widthStep + j +2];
        
            iplpic->imageData[i * iplpic->widthStep + j + 2] = tempR; //+2
            iplpic->imageData[i * iplpic->widthStep + j + 1] =tempG;
            iplpic->imageData[i * iplpic->widthStep + j] = tempB; //+0
        }
    }
    
    return iplpic;
};