//
//  MainWindowController.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 5/23/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface MainWindowController : NSWindowController
{
    NSView* glView;
}

@property (nonatomic, retain) NSView* glView;

@end
