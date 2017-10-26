//
//  AppDelegate.m
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/4/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "AppDelegate.h"
//#import <QTKit/QTKit.h>

static NSString* getWorkingDirectory()
{
    return [NSString stringWithFormat:@"%s", getcwd(NULL, 0)];
}

static NSArray* getConfigFilesInDirectory(NSString* directoryPath)
{
    NSArray* directoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:directoryPath error: nil];
    NSPredicate *filter = [NSPredicate predicateWithFormat:@"self ENDSWITH '.cfg'"];
    NSArray* content = [directoryContents filteredArrayUsingPredicate:filter];
    if ([content count] <= 0)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"OK"];
        [alert setMessageText:@"Error! Configuration not found."];
        [alert setInformativeText:@"No tracking configuration files were found in the Resources folder!"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        exit(-1);
    }
    return content;
}

static void changeDirectory(NSTextField* f)
{
    NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
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
}

@implementation AppDelegate

@synthesize tracking;
@synthesize glView;
@synthesize tracker;
@synthesize window;
@synthesize currentTrackingConfiguration;
@synthesize selectedTrackingConfiguration;
@synthesize trackingConfiguration;
@synthesize mainWindowController;

@synthesize fpsLabel;
@synthesize statusLabel;
@synthesize infoLabel;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    changeDirectory(infoLabel);
    NSArray* configFiles = getConfigFilesInDirectory(getWorkingDirectory());
    [trackingConfiguration removeAllItems];
    [trackingConfiguration addItemsWithTitles:configFiles];
    
    [trackingConfiguration selectItemWithTitle:@"Facial Features Tracker - High.cfg"]; //select default config
    
    MainWindowController* controller = [[MainWindowController alloc] initWithWindow:self.window];
    controller.glView = self.glView;
    self.mainWindowController = controller;
    [self.window setAcceptsMouseMovedEvents:YES];
    
    [self.window setStyleMask:[self.window styleMask] | NSResizableWindowMask];
    
    self.tracker = [[TrackerWrapper alloc] init];
    tracking = NO;
}

-(IBAction)trackFromCamClick:(id)sender
{
    if (tracking)
    {
        [self.tracker stopTracker];
        tracking = NO;
    }
    
    selectedTrackingConfiguration = [[trackingConfiguration selectedItem] title];
    if (selectedTrackingConfiguration != currentTrackingConfiguration)
    {
        currentTrackingConfiguration = selectedTrackingConfiguration;
        [self.tracker initTracker:glView withConfigurationFile:currentTrackingConfiguration andInformer:self.createInformer];
    }
    
    [self.tracker startTrackingFromCam];
    tracking = YES;
}

-(IBAction)trackFromVideoClick:(id)sender
{
    if (tracking)
    {
        [self.tracker stopTracker];
        tracking = NO;
    }
    
    NSOpenPanel* openVideoDialog = [NSOpenPanel openPanel];
    openVideoDialog.canCreateDirectories = NO;
    openVideoDialog.canChooseDirectories = NO;
    openVideoDialog.allowsMultipleSelection = NO;
    openVideoDialog.canChooseFiles = YES;
    openVideoDialog.title = @"Open video";
    NSArray* fileTypes = [NSArray arrayWithObjects:@"avi", @"AVI", @"mp4",nil];
    openVideoDialog.allowedFileTypes = fileTypes;
    NSString* videoPath;
    if ([openVideoDialog runModal] == NSOKButton)
    {
        NSURL* selection = [[openVideoDialog URLs] objectAtIndex:0];
        videoPath = [selection.path stringByResolvingSymlinksInPath];
        [openVideoDialog close];
    }
    else
    {
        return;
    }
    
    selectedTrackingConfiguration = [[trackingConfiguration selectedItem] title];
    if (selectedTrackingConfiguration != currentTrackingConfiguration)
    {
        currentTrackingConfiguration = selectedTrackingConfiguration;
        [self.tracker initTracker:glView withConfigurationFile:currentTrackingConfiguration andInformer:self.createInformer];
    }
    
    [self.tracker startTrackingFromVideo:videoPath];
    tracking = YES;
}

-(IBAction)trackInImageClick:(id)sender
{
    if (tracking)
    {
        [self.tracker stopTracker];
        tracking = NO;
    }
    
    selectedTrackingConfiguration = [[trackingConfiguration selectedItem] title];
    if (selectedTrackingConfiguration != currentTrackingConfiguration)
    {
        currentTrackingConfiguration = selectedTrackingConfiguration;
        [self.tracker initTracker:glView withConfigurationFile:currentTrackingConfiguration andInformer:self.createInformer];
    }
    
    [self.tracker startTrackingFromImage];
    tracking = YES;
}

-(IBAction)stopClick:(id)sender
{
    if (tracking) {
        [self.tracker stopTracker];
        tracking = NO;
    }
}

- (Informer*) createInformer
{
    Informer* informer = [Informer alloc];
    informer.fpsLabel = self.fpsLabel;
    informer.infoLabel = self.infoLabel;
    informer.statusLabel = self.statusLabel;
    return informer;
}

@end
