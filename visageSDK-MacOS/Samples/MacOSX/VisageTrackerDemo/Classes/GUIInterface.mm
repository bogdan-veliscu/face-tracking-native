//
//  GUIInterface.cpp
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 5/9/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#include "GUIInterface.h"
#import "NotificationWindow.h"
#import "InstructionWindow.h"

#define INSTRUCTIONS @"Please adjust the shape mask to the face. For examples of correctly fitted masks please consult the tutorial.\n\nUse mouse to drag regions of the mask.\nRight mouse button moves the whole mask.\nR key + left mouse button zooms the mask.\nR key + right mouse button rotates the mask.\nT key + right mouse button tilts the mask."

#define CUSTOM_SIDE_Y 500
#define CUSTOM_SIDE_X 600

GUIInterface::GUIInterface(CustomGLView* glView)
{
    this->glView = glView;
    this->vt2 = NULL;
};

char* GUIInterface::ChooseSuFileName()
{
    //
    return NULL;
};

void GUIInterface::ManuallyAdjustShapeMask(float *face_translation, float *face_rotation, float *face_scale, float *face_shape)
{
    //
};

int GUIInterface::selectMainFacialFeatures(IplImage* input, FDP* output)
{
    //
    return 0;
};
void GUIInterface::signalWhenUserReady(char *userMessage,bool *readyFlag)
{
    //
}