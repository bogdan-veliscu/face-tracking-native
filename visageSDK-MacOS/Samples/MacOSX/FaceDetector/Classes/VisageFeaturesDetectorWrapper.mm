//
//  VisageFeaturesDetectorWrapper.m
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "VisageFeaturesDetectorWrapper.h"
#import "Common.h"

#include <cv.h>
#include "VisageDrawing.h"

#define MAX_SIZE 768

using namespace VisageSDK;

@implementation VisageFeaturesDetectorWrapper

- (void)initDetector
{
    //initialize licensing
    //example how to initialize license key
    //initializeLicenseManager("license-key-filename.vlc");
    
    visageFeaturesDetector = new VisageFeaturesDetector();
    NSString* resourcePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/bdtsdata"];
    visageFeaturesDetector->Initialize([resourcePath UTF8String]);
}

- (NSImage*)drawFeatures:(NSImage*)image
{
    // if detector not initialized return the input image
    if (visageFeaturesDetector == NULL)
        return image;
    
    // convert input NSImage to IplImage
    IplImage* input = IplImageFromNSImage(image);
    
    // create the output image by copying the input image
    IplImage* output = cvCreateImage(cvGetSize(input), input->depth, input->nChannels);
    cvCopyImage(input, output);
    
    // resize input image
    float scale = (float)MAX_SIZE / MAX(input->width, input->height);
    IplImage* resizedInput = cvCreateImage(cvSize(input->width * scale, input->height * scale), input->depth, input->nChannels);
    cvResize(input, resizedInput);
    
    // detect features
    int maxFaces = 100;
    FaceData* data = new FaceData[maxFaces];
    int faces = -1;
    faces = visageFeaturesDetector->detectFacialFeatures((VsImage*)resizedInput, data, maxFaces);
    
    if(faces > 0)
    {
        // draw features into the output image
        for(int i = 0; i < faces; i++)
            VisageDrawing::drawResults((VsImage*)resizedInput, &data[i]);
    }
    
    // convert resizedInput to 4-channels image in order to draw logo
    IplImage* iplImageBGRA = cvCreateImage(cvSize(resizedInput->width,resizedInput->height),IPL_DEPTH_8U,4);
    switch(resizedInput->nChannels)
    {
        case 4: //BGRA input image
            cvCopy(resizedInput, iplImageBGRA);
            break;
        case 3: //BGR input image
            cvCvtColor(resizedInput, iplImageBGRA, CV_BGR2BGRA);
            break;
    }
    
    // load logo image by the given path
    NSString *logoPath = [[NSBundle mainBundle] pathForResource:@"logo" ofType:@"png"];
    NSImage *logoImage = [[NSImage alloc] initWithContentsOfFile:logoPath];
    IplImage* logo = IplImageFromNSImage(logoImage);
    
    //draw logo
    if (logo != NULL)
        VisageDrawing::drawLogo((VsImage*)iplImageBGRA, (VsImage*)logo);
    
    // convert the output IplImage to NSImage
    NSImage* returnImage = NSImageFromIplImage(iplImageBGRA);
    
    // release the input and output IplImages
    cvReleaseImage(&input);
    cvReleaseImage(&resizedInput);
    cvReleaseImage(&output);
    cvReleaseImage(&iplImageBGRA);
    
    // return
    return returnImage;
}

@end
