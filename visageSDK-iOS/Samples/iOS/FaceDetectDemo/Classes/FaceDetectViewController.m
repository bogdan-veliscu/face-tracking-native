#import "FaceDetectViewController.h"
#import "VisageFeaturesDetectorWrapper.h"
#import "VisageFaceRecognitionWrapper.h"
#import "MobileCoreServices/UTCoreTypes.h"
#import "ConversionFunctions.h"

@implementation FaceDetectViewController

@synthesize detectImage;
@synthesize detectToolbar;
@synthesize imgPicker;
@synthesize popover;
@synthesize detectorWrapper;
@synthesize recognitionWrapper;
@synthesize logo;

bool inited = false;

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    
    [super viewDidLoad];

    // init image picker
    if (imgPicker == nil)
    {
        self.imgPicker = [[UIImagePickerController alloc] init];
        self.imgPicker.delegate = self;
        self.imgPicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
        self.imgPicker.mediaTypes = [[NSArray alloc] initWithObjects: (NSString *) kUTTypeImage, nil];
    }
    
    // init face detector
    if (detectorWrapper == nil)
    {
        self.detectorWrapper = [[VisageFeaturesDetectorWrapper alloc] init];
        [self.detectorWrapper initDetector];
    }
    
    //load logo image by the given path
    NSString *logoPath = [[NSBundle mainBundle] pathForResource:@"logo" ofType:@"png"];
    UIImage *logoImage = [UIImage imageWithContentsOfFile:logoPath];
    logo = IplImageFromUIImage(logoImage, 4);
    
    self.detectorWrapper->logo = logo;
}

-(IBAction)loadImageAction:(id)sender {
    
    mode = DETECTION_MODE;
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad){
        self.popover = [[UIPopoverController alloc] initWithContentViewController:self.imgPicker];
        [self.popover presentPopoverFromRect:CGRectMake(0.0, 0.0, 400.0, 400.0) inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
    }
    else {
        [self presentViewController:self.imgPicker animated:YES completion:nil];
    }
}

- (IBAction)recognizeAction:(id)sender {
    
    //init face recognition
    if(recognitionWrapper == nil)
    {
        self.recognitionWrapper = [[VisageFaceRecognitionWrapper alloc] init];
        inited = [self.recognitionWrapper initRecognition: self.detectorWrapper->visageFeaturesDetector];
        
        self.recognitionWrapper->logo = logo;
    }
    
    if (inited)
    {
        NSString *path = [[NSBundle mainBundle] pathForResource:@"/Famous Actors/match/ScarlettJohansson" ofType:@"jpg"];
        UIImage *image = [UIImage imageWithContentsOfFile:path];
        
        UIImage* recognitionImage = [self.recognitionWrapper recognizeFace : image];
        [imageView setImage:recognitionImage];
    }
}

- (void) imagePickerController:(UIImagePickerController *)picker didFinishPickingImage:(UIImage *)image
                      editingInfo:(NSDictionary *)editingInfo
{
    
    [self dismissViewControllerAnimated:YES completion:nil];
    
  
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        [popover dismissPopoverAnimated:YES];
    }
    
    if(mode == DETECTION_MODE)
    {
        // detect features from loaded image
        UIImage* featuresImage = [detectorWrapper drawFeatures:image];
        [imageView setImage:featuresImage];
    }
}

- (void) imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


@end
