//
//  OpenGLInterface.h
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/30/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "CustomGLView.h"
#include "visageVision.h"


#define MEASURE_FRAMES 10

using namespace VisageSDK;

/**
 * Used as an adapter for Objective-C class CustomGLView and passing C++ method calls to it.
 */
class OpenGLInterface
{
private:
    
    /**
     * Custom OpenGl view used for tracking rendering.
     */
    CustomGLView* view;
public:
    
    /** Constructor. 
     * Initializes instance of OpenGL interface with the specified CustomGLView class instance.
     *
     * @param view CustomGLView class instance
     */
    OpenGLInterface(CustomGLView* view);
    
    /**
     * Method used for setting OpenGLContext.
     */
    void setOpenGLContext();
    
    /**
     * Method used for getting OpenGL view dimensions.
     * @param width Reference to width dimension.
     * @param height Reference to height diemnsion
     */
    void checkOpenGLSize(unsigned int &width, unsigned int &height);
    
    /**
     * Method used for swapping OpenGL buffers.
     */
    void swapOpenGLBuffers();

};
