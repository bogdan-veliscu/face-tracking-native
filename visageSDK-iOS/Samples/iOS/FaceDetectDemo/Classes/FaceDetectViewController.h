#import <UIKit/UIKit.h>

#define DETECTION_MODE 1
#define RECOGNITION_MODE 2

@class VisageFeaturesDetectorWrapper;
@class VisageFaceRecognitionWrapper;

/**
 * Implements default view controller and GUI actions.
 *
 * Most method calls are forwarded to the VisageFeaturesDetectorWrapper object that contains specific implementions.
 *
 */
@interface FaceDetectViewController : UIViewController <UIAlertViewDelegate,UINavigationControllerDelegate, UIImagePickerControllerDelegate, UIPopoverControllerDelegate> {
    
    /**
     * Toolbar for controlling the app.
     * 
     */
	IBOutlet UIToolbar *detectToolbar;
    
    /**
     * Image picker for selecting an image file from album.
     *
     */
    UIImagePickerController *imgPicker;
    
    /**
     * Popover for the image picker.
     *
     */
    UIPopoverController *popover;

    /**
     * View for displaying results.
     * 
     */
    IBOutlet UIImageView *imageView;
    
    /**
     * Wrapper for the VisageFeaturesDetector.
     */
    VisageFeaturesDetectorWrapper *detectorWrapper;
    
    /**
     * Wrapper for the VisageFeaturesRecognition.
     */
    VisageFaceRecognitionWrapper *recognitionWrapper;
    
    int mode;
    
    /**
     * Logo image used both for the VisageFeaturesDetector and VisageFeaturesRecognition.
     */
    IplImage* logo;
}

@property (nonatomic, retain) UIImage *detectImage;
@property (nonatomic, retain) IBOutlet UIToolbar *detectToolbar;
@property (nonatomic, retain) UIImagePickerController *imgPicker;
@property (nonatomic, retain) UIPopoverController *popover;
@property (nonatomic, retain) VisageFeaturesDetectorWrapper *detectorWrapper;
@property (nonatomic, retain) VisageFaceRecognitionWrapper *recognitionWrapper;
@property (nonatomic) IplImage* logo;

/**
 * Method for startig tracking from an image.
 * 
 */
-(IBAction)loadImageAction:(id)sender;

-(IBAction)recognizeAction:(id)sender;

@end
