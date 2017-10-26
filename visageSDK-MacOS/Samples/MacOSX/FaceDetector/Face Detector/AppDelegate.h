//
//  AppDelegate.h
//  Face Detector
//
//  Created by Josip Ivekovic on 6/14/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "VisageFeaturesDetectorWrapper.h"

/**
 * Class used as a view controller for the Face Detect application.
 */
@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    /**
     * Pointer to detector wrapper used in face detection.
     */
    VisageFeaturesDetectorWrapper* detectorWrapper;
    /**
     * Pointer to application main window.
     */
    NSWindow* window;
    /**
     * Pointer to a filed for displaying images.
     */
    NSImageCell* imageCell;
}

/**
 * Pointer to detector wrapper used in face detection.
 */
@property (nonatomic, retain) VisageFeaturesDetectorWrapper* detectorWrapper;
/**
 * Pointer to application main window.
 */
@property (nonatomic, retain) IBOutlet NSWindow *window;
/**
 * Pointer to a filed for displaying images.
 */
@property (nonatomic, retain) IBOutlet NSImageCell *imageCell;

- (IBAction) openClick:(id)sender;

@end
