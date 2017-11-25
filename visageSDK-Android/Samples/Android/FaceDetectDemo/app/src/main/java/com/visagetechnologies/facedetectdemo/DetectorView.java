package com.visagetechnologies.facedetectdemo;

import android.content.Context;
import android.hardware.Camera;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/** Base class used for defining surface for displaying tracking results.
 * Derives from SurfaceView class provided by Android. SurfaceView class provides basic implementation that handles layout of and interaction with user interface.
 * Implements SurfaceHolder.Callback interface. SurfaceHolder.Callback interface is used to obtain information about changes to the surface object.
 */
public abstract class DetectorView extends SurfaceView implements SurfaceHolder.Callback 
{
	int _width;
	int _height;
	DetectorGLSurfaceView _tview;
	boolean isRunning;
	Thread _thread;
	DetectorRenderer _trenderer;
	Camera _camera;

	public DetectorView(Context context) 
	{
		super(context);	
	}

	/** Called to set GLSurfaceView used for rendering with OpenGL.
	 * 
	 * @param tview GLSurfaceView object.
	 */
	public void setGLView(DetectorGLSurfaceView tview)
	{
		_tview = tview;
	}

	boolean hasFrontCamera()
	{
		return false;
	}

}
