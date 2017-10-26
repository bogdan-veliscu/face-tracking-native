//
//  Informer.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 6/4/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 * Displays tracking information.
 */
@interface Informer : NSObject
{
    /**
     * Label with fps information.
     */
    UILabel* fpsLabel;
    /**
     * Label with tracking information.
     */
    UILabel* infoLabel;
    /**
     * Label with tracking status.
     */
    UILabel* statusLabel;
}

/**
 * Label with fps information.
 */
@property (nonatomic, retain) UILabel* fpsLabel;
/**
 * Label with tracking information.
 */
@property (nonatomic, retain) UILabel* infoLabel;
/**
 * Label with tracking status.
 */
@property (nonatomic, retain) UILabel* statusLabel;

/**
 * Method used for displaying tracking information and status.
 * @param fps FPS information.
 * @param info Tracking inforamtion.
 * @param status Tracking status.
 */
- (void) showTrackingFps:(NSString*) fps Info:(NSString*) info andStatus:(NSString*) status;

@end
