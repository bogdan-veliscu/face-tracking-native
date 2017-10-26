//
//  CustomGLView.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/9/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

/**
 Controls OpenGl view used for tracking output.
 */
@interface CustomGLView : NSOpenGLView
{
    /**
     * Flag that specifies whether events are enabled for the current view.
     */
    bool eventsEnabled;
}

/**
 * eventsEnabled
 * Flag that specifies whether events are enabled for the current view.
 */
@property bool eventsEnabled;



@end