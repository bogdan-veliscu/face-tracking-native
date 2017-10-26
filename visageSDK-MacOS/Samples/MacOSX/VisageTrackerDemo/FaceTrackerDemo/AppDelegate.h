//
//  AppDelegate.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/4/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "CustomGLView.h"
#import "TrackerWrapper.h"
#import "MainWindowController.h"
#import "Informer.h"

/**
 * Class used as an app view controller.
 */
@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    bool tracking;
    
    NSWindow *window;
    CustomGLView* glView;
    TrackerWrapper* tracker;
    NSString* currentTrackingConfiguration;
    NSString* selectedTrackingConfiguration;
    NSPopUpButton* trackingConfiguration;
    NSWindowController* mainWindowController;
    
    NSTextField* fpsLabel;
    NSTextField* statusLabel;
    NSTextField* infoLabel;
}

@property (nonatomic, retain) IBOutlet NSWindow *window;
@property (nonatomic, retain) IBOutlet CustomGLView *glView;
@property (nonatomic, retain) TrackerWrapper *tracker;
@property (nonatomic, retain) NSString *currentTrackingConfiguration;
@property (nonatomic, retain) NSString *selectedTrackingConfiguration;
@property (nonatomic, retain) IBOutlet NSPopUpButton *trackingConfiguration;
@property (nonatomic, retain) NSWindowController* mainWindowController;
@property (nonatomic) bool tracking;

@property (nonatomic, retain) IBOutlet NSTextField* fpsLabel;
@property (nonatomic, retain) IBOutlet NSTextField* statusLabel;
@property (nonatomic, retain) IBOutlet NSTextField* infoLabel;

/**
 * Event handler for selecting "Tracking from cam" menu item.
 */
- (IBAction)trackFromCamClick:(id)sender;
/**
 * Event handler for selecting "Tracking from video" menu item.
 */
- (IBAction)trackFromVideoClick:(id)sender;
/**
 * Event handler for selecting "Tracking from image" menu item.
 */
- (IBAction)trackInImageClick:(id)sender;
/**
 * Event handler for selecting "Stop tracking" menu item.
 */
- (IBAction)stopClick:(id)sender;
/**
 * Method used for creating and initializing a new informer.
 */
- (Informer*) createInformer;

@end
