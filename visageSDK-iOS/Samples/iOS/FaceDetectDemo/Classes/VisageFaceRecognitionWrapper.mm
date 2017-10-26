#import "VisageFaceRecognitionWrapper.h"
#import "Utilities.h"
#import "VisageDrawing.h"
#import "ConversionFunctions.h"
#import <UIKit/UIKit.h>

#include "sys/timeb.h"

#define MAX_FACES 100

using namespace VisageSDK;

@implementation VisageFaceRecognitionWrapper

- (bool) initRecognition : (VisageFeaturesDetector*) detector
{
    if (vfr)
        return true;
    
    else
    {
        NSString* resourcePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/bdtsdata/NN"];
        
        vfr = new VisageFaceRecognition([resourcePath UTF8String]);
        vfd = detector;
    
        int success = [self createGallery];
        
        if (!success)
        {
            return false;
        }
        
        if (!(vfr->is_initialized))
        {
            return false;
        }
       
        return true;
    }
}

- (int) createGallery
{
    if (!vfd)
        return 0;
    
    NSString* path = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/Famous Actors/gallery"];
    NSArray* dirFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:nil];
    NSArray* jpgFiles = [dirFiles filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH '.jpg'"]];
    
    if ([jpgFiles count] == 0)
        return 0;
    
    for (int i = 0; i < [jpgFiles count]; i++)
    {
        NSString* filePath = [NSString stringWithFormat:@"%@/%@", path, jpgFiles[i]];
        UIImage* nsImage = [[UIImage alloc] initWithContentsOfFile: filePath];

        IplImage* iplImage = IplImageFromUIImage(nsImage, 3);
    
        FaceData data;
        
        int n_faces = vfd->detectFacialFeatures((VsImage*) iplImage, &data);
        
        NSString* name = [jpgFiles[i] stringByReplacingOccurrencesOfString:@".jpg" withString:@""];
        
        if(n_faces > 0)
            vfr->addDescriptor((VsImage*) iplImage, &data, [name UTF8String]);
        
        cvReleaseImage(&iplImage);
    }
    
    return 1;
}

- (UIImage*) recognizeFace:(UIImage*) uiImage
{
    // if detector not initialized return the input image
    if (!vfd)
        return uiImage;
    
    IplImage* iplImage = IplImageFromUIImage(uiImage, 3);
    
    FaceData* data = new FaceData[MAX_FACES];
    int n_faces = vfd->detectFacialFeatures((VsImage*) iplImage, data, MAX_FACES
                                            );
    
    if(n_faces > 0)
    {
        for (int i = 0; i < n_faces; i++)
        {
            const int DESCRIPTOR_SIZE = vfr->getDescriptorSize();
            short* descriptor = new short[DESCRIPTOR_SIZE];
            const int TOP_N_FACES = 3;
            const char* names[TOP_N_FACES];
            float sim[TOP_N_FACES];
            const float SIMILARITY_THRESHOLD = 0.45f;
  
            vfr->extractDescriptor(&data[i], (VsImage*)iplImage, descriptor);
            vfr->recognize(descriptor, TOP_N_FACES, names, sim);
        
            printf("name: %s, similiratiy: %f\n", names[0], sim[0]);

            delete[] descriptor;
            
            char result[1024];
            sprintf(result, "%s %f", names[0], sim[0]);
        
            if (sim[0] > SIMILARITY_THRESHOLD)
                VisageDrawing::drawMatchingIdentity(iplImage, data[i], result);
            else
                VisageDrawing::drawMatchingIdentity(iplImage, data[i], "?");
        }
    }
    
    // convert iplImage to 4-channels image in order to draw logo
    IplImage* iplImageBGRA = cvCreateImage(cvSize(iplImage->width,iplImage->height),IPL_DEPTH_8U,4);
    cvCvtColor(iplImage, iplImageBGRA, CV_BGR2BGRA);
    
    //draw logo
    if (logo)
        VisageDrawing::drawLogo((VsImage*)iplImageBGRA, (VsImage*)logo);
    
    // convert the output IplImage to UIImage
    UIImage* returnImage = UIImageFromIplImage(iplImageBGRA);
    
    cvReleaseImage(&iplImage);
    cvReleaseImage(&iplImageBGRA);
    delete[] data;
    
    return returnImage;
}


@end

