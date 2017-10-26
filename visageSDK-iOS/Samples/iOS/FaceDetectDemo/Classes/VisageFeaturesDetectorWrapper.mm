#include <cv.h>
#import "VisageFeaturesDetectorWrapper.h"
#import "Utilities.h"
#import "Common.h"
#import "VisageDrawing.h"
#import "ConversionFunctions.h"
#define MAX_SIZE 768

using namespace VisageSDK;

@implementation VisageFeaturesDetectorWrapper

- (void)initDetector
{
    NSLog(@"init detector");
    
    //initialize licensing
    //example how to initialize license key
    //initializeLicenseManager("license-key-filename.vlc");
    
    visageFeaturesDetector = new VisageFeaturesDetector();
    visageFeaturesDetector->Initialize([Utilities bundlePath:@"bdtsdata"].UTF8String);
}

- (UIImage*)drawFeatures:(UIImage *)image
{
    // if detector not initialized return the input image
    if (visageFeaturesDetector == NULL)
        return image;
    
    // convert input UIImage to IplImage
    IplImage* input = IplImageFromUIImage(image, 4);
    
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
    
    // draw features into the output image
    if(faces > 0)
    {
        for(int i = 0; i < faces; i++)
            VisageDrawing::drawResults((VsImage*)resizedInput, &data[i]);
    }
    
    //draw logo
    if(logo != NULL)
        VisageDrawing::drawLogo((VsImage*)resizedInput, (VsImage*)logo);
    
    // convert the output IplImage to UIImage
    UIImage* returnImage = UIImageFromIplImage(resizedInput);
    
    // release the input and output IplImages
    cvReleaseImage(&input);
    cvReleaseImage(&resizedInput);
    cvReleaseImage(&output);
    
    delete[] data;
    // return
    return returnImage;
}

@end
