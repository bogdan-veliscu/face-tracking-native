#include <cv.h>
#import "DemoFrameGrabber.h"
#import "HelperFunctions.h"

using namespace VisageSDK;


DemoFrameGrabber::DemoFrameGrabber(void)
{
    NSOpenPanel* openPictureDialog = [NSOpenPanel openPanel];
    openPictureDialog.canCreateDirectories = NO;
    openPictureDialog.canChooseDirectories = NO;
    openPictureDialog.allowsMultipleSelection = NO;
    openPictureDialog.canChooseFiles = YES;
    openPictureDialog.title = @"Open image file";
    NSArray* fileTypes = [NSArray arrayWithObjects:@"bmp", @"jpg", @"jpeg", @"gif", @"png", nil];
    openPictureDialog.allowedFileTypes = fileTypes;
    NSString* path;
    if ([openPictureDialog runModal] == NSOKButton)
    {
        NSURL* selection = [[openPictureDialog URLs] objectAtIndex:0];
        path = [selection.path stringByResolvingSymlinksInPath];
        [openPictureDialog close];
    }
    else
    {
        throw int(20); //There is no reason for this to be either int nor 20. It was chosen on a whim.
    }
    
    
	NSImage *image = [[NSImage alloc] initWithContentsOfFile:path];
    
    NSImageRep *rep = [[image representations] objectAtIndex: 0];
    // If you think you might get something other than a bitmap image representation,
    // check for it here.
    
    NSSize size2 = NSMakeSize ([rep pixelsWide], [rep pixelsHigh]);
    
    int w = size2.width;
    w = (w / 4) * 4;
    int h = size2.height;
    h = (h / 4) * 4;
    
    NSSize size;
    size.width = w;
    size.height = h;
    [image setSize: size];
    
    NSImage *resizedImage = [[NSImage alloc] initWithSize: size];
    [resizedImage lockFocus];
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
    [image drawAtPoint:NSZeroPoint fromRect:NSMakeRect(0, 0, size.width, size.height) operation:NSCompositeCopy fraction:1.0];
    [resizedImage unlockFocus];
	    
    frame = IplImageFromNSImage(resizedImage);//CreateIplImageFromNSImage(image, 4);
    
    width = frame->width;
    height = frame->height;
    int nChannels = frame->nChannels;
    switch(nChannels)
    {
        case 1:
            format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
            break;
        case 3:
            format = VISAGE_FRAMEGRABBER_FMT_RGB;
            break;
        case 4:
            format = VISAGE_FRAMEGRABBER_FMT_RGBA;
            break;
        default:
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            [alert setMessageText:@"Error! Unsupported image format."];
            [alert setInformativeText:@"Image file contains unsupported image format!"];
            [alert setAlertStyle:NSWarningAlertStyle];
            [alert runModal];
            throw int(20);
    }
    origin = VISAGE_FRAMEGRABBER_ORIGIN_TL;
};

unsigned char *DemoFrameGrabber::GrabFrame(long &timeStamp)
{
    timeStamp = -1;

    unsigned char* ret = (unsigned char*) frame->imageData;

    return ret;
}

DemoFrameGrabber::~DemoFrameGrabber(void)
{
    // release loaded image
    if (frame)
        cvReleaseImage(&frame);
}
