//
//  GUIInterface.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 5/9/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#ifndef __FaceTrackerDemo__GUIInterface__
#define __FaceTrackerDemo__GUIInterface__

#import <Foundation/Foundation.h>
#import "CustomGLView.h"
#include "TrackerGUIInterface.h"
#include "highgui.h"
#include "FDP.h"
#include "OpenGLInterface.h"

#endif /* defined(__FaceTrackerDemo__GUIInterface__) */

#define SU_FILE_EXTENSION @"su"

using namespace VisageSDK;

/**
 * Used as an adapter for Objective-C classes and for passing C++ method calls to them.
 */
class GUIInterface : public TrackerGUIInterface
{
private:
    /**
     * Custom Gl view used for tracking rendering.
     */
    CustomGLView* glView;
public:
    /** Constructor. 
     * Initializes instance of GUI interface with the specified CustomGLView class instance.
     *
     * @param glView Instance of Objective-C CustomGLView class used for initialization.
     */
    GUIInterface(CustomGLView* glView);
    /**
     * Method used for choosing profile file name.
     */
    virtual char* ChooseSuFileName();
    /**
     * Method for displaying messages to the user.
     * @param userMessage User message text.
     * @param caption Message caption.
     * @param type Message type.
     */
    virtual void displayMessage(char *userMessage,char *caption,int type);
    /**
     * Method used for manual mask adjustment in semi-automatic setup.
     * @param face_translation Pointer to vector containing information about face translation.
     * @param face_rotation Pointer to vector containing information about face rotation.
     * @param face_scale Pointer to vector containing information about face scale.
     * @param face_shape Pointer to vector containing information about face shape.
     */
    virtual void ManuallyAdjustShapeMask(float *face_translation, float *face_rotation, float *face_scale, float *face_shape);
    /**
     * Method used for displaying user message and signaling when the user is ready to continue.
     * @param userMessage User message text.
     * @param readyFlag Pointer to a flag signaling when the user is ready.
     */
    virtual void signalWhenUserReady(char *userMessage,bool *readyFlag);
    /**
     * Method used for selection of main facial features.
     * @param input Image displayed to the user in manual face detection.
     * @param output Main facial point in FDP format.
     * @return Bit field signaling which points were selected.
     */
    virtual int selectMainFacialFeatures(IplImage* input, FDP* output);
    
    OpenGLInterface* glInterface;
};
