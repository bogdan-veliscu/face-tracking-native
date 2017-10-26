//
//  CustomGLView.m
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/9/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import "CustomGLView.h"

#define R_KEY 15
#define T_KEY 17

@interface CustomGLView()

@end

@implementation CustomGLView


@synthesize eventsEnabled;

-(id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
	
    eventsEnabled = false;
    
	// The GL Context must be active before you can use OpenGL functions
	NSOpenGLContext *glcontext = [self openGLContext];
	[glcontext makeCurrentContext];

	//Add your OpenGL init code here
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	return self;
}

@end
