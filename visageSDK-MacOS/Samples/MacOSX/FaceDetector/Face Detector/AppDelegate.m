//
//  AppDelegate.m
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate

@synthesize detectorWrapper;
@synthesize window;
@synthesize imageCell;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    NSString * resourcePath = [[NSBundle mainBundle] resourcePath];
    NSFileManager *filemgr = [NSFileManager defaultManager];
    if ([filemgr changeCurrentDirectoryPath:resourcePath] == NO)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"OK"];
        [alert setMessageText:@"Error! Folder not found."];
        [alert setInformativeText:@"Resources folder not found!"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        exit(-1);
    }
    
    if (detectorWrapper == nil)
    {
        self.detectorWrapper = [[VisageFeaturesDetectorWrapper alloc] init];
        [self.detectorWrapper initDetector];
    }
}

- (IBAction) openClick:(id)sender
{
    NSOpenPanel* openImageDialog = [NSOpenPanel openPanel];
    openImageDialog.canCreateDirectories = NO;
    openImageDialog.canChooseDirectories = NO;
    openImageDialog.allowsMultipleSelection = NO;
    openImageDialog.canChooseFiles = YES;
    openImageDialog.title = @"Open image";
    NSArray* fileTypes = [NSArray arrayWithObjects:@"jpg", @"gif", @"bmp", @"png", @"jpeg", @"gif", nil];
    openImageDialog.allowedFileTypes = fileTypes;
    NSString* imagePath;
    if ([openImageDialog runModal] == NSOKButton)
    {
        NSURL* selection = [[openImageDialog URLs] objectAtIndex:0];
        imagePath = [selection.path stringByResolvingSymlinksInPath];
        [openImageDialog close];
    }
    else
    {
        return;
    }
    
    NSImage* image = [[NSImage alloc] initWithContentsOfFile:imagePath];
    NSImage* displayImage = [detectorWrapper drawFeatures:image];
    
    [self.imageCell setImage:displayImage];
}

@end
