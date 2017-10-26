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

// helper function for converting UIImage to IplImage
IplImage *IplImageFromUIImage(UIImage *image, int nChannels) {
    
    UIImage *rotatedImage = image;
    if (image.imageOrientation == UIImageOrientationRight)
        rotatedImage = [[UIImage alloc] initWithCGImage:image.CGImage scale:1.0 orientation:UIImageOrientationUp];
    
    CGImageRef imageRef = rotatedImage.CGImage;
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    IplImage *iplimage = cvCreateImage(cvSize(rotatedImage.size.width, rotatedImage.size.height), IPL_DEPTH_8U, 4);
    CGContextRef contextRef = CGBitmapContextCreate(iplimage->imageData, iplimage->width, iplimage->height,
                                                    iplimage->depth, iplimage->widthStep,
                                                    colorSpace, kCGImageAlphaPremultipliedLast|kCGBitmapByteOrderDefault);
    CGContextDrawImage(contextRef, CGRectMake(0, 0, rotatedImage.size.width, rotatedImage.size.height), imageRef);
    
    
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
    NSLog(@"Image orientation: %d", image.imageOrientation);
    
    if (image.imageOrientation == UIImageOrientationRight)
    {
        IplImage *transposedRet = cvCreateImage(cvSize(ret->height, ret->width), ret->depth, ret->nChannels);
        cvFlip(ret);
        cvTranspose(ret, transposedRet);
        
        cvReleaseImage(&ret);
        return transposedRet;
    }
    
    return ret;
}

// helper function for creating an UIImage from an IplImage
UIImage* UIImageFromIplImage(IplImage* image) {
    
    // convert to rgb
    IplImage* convertedImage = cvCreateImage(cvGetSize(image), image->depth, 4);
    cvCvtColor(image, convertedImage, CV_BGR2RGBA);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    NSData *data = [NSData dataWithBytes:convertedImage->imageData length:convertedImage->imageSize];
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((CFDataRef)data);
    CGImageRef imageRef = CGImageCreate(convertedImage->width, convertedImage->height,
                                        convertedImage->depth, convertedImage->depth * convertedImage->nChannels, convertedImage->widthStep,
                                        colorSpace, kCGImageAlphaPremultipliedLast|kCGBitmapByteOrderDefault,
                                        provider, NULL, false, kCGRenderingIntentDefault);
    UIImage *ret = [UIImage imageWithCGImage:imageRef scale:1.0 orientation:UIImageOrientationUp];
    CGImageRelease(imageRef);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    
    // release convertedImage
    cvReleaseImage(&convertedImage);
    return ret;
}
