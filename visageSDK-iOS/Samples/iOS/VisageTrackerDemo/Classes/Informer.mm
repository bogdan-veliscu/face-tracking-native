//
//  Informer.m
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 6/4/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "Informer.h"

@implementation Informer

@synthesize fpsLabel;
@synthesize infoLabel;
@synthesize statusLabel;

- (void) showTrackingFps:(NSString *)fps Info:(NSString *)info andStatus:(NSString *)status
{
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        
        [self.fpsLabel      setText:fps];
        [self.infoLabel     setText:info];
        [self.statusLabel   setText:status];
        
    }];
}

@end


