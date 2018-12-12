// FaceDetector.h : main header file for the FaceDetector application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CFaceDetectorApp:
// See FaceDetector.cpp for the implementation of this class
//

class CFaceDetectorApp : public CWinApp
{
public:
	CFaceDetectorApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CFaceDetectorApp theApp;