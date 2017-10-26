//
//  OpenGLInterface.m
//  FaceTrackerDemo
//
//  Created by Josip Ivekovic on 4/30/13.
//  Copyright (c) 2013 Josip Ivekovic. All rights reserved.
//

#include "OpenGLInterface.h"
#include "VisageRendering.h"


OpenGLInterface::OpenGLInterface(CustomGLView* view)
{
    this->view = view;
};

void OpenGLInterface::setOpenGLContext()
{
    NSOpenGLView* view = this->view;
    NSOpenGLContext* context = [view openGLContext];
    [context makeCurrentContext];
    
};

void OpenGLInterface::checkOpenGLSize(unsigned int &width, unsigned int &height)
{
    NSOpenGLView* view = this->view;
    NSSize size = view.frame.size;
    width = size.width;
    height = size.height;
};

void OpenGLInterface::swapOpenGLBuffers()
{
    glSwapAPPLE();
};


