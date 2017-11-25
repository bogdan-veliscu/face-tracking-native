package com.visagetechnologies.facedetectdemo;

import android.content.Context;
import android.opengl.GLSurfaceView;

/** Implementation of GLSurfaceView class provided by Android.
 * 
 *  Creates and sets OpenGL renderer that contains the actual rendering code to render on this view.
 */
public class DetectorGLSurfaceView extends GLSurfaceView
{
	private DetectorRenderer _trenderer;
	
	/** Constructor.
	 * 
	 * @param context global information about android application environment.
	 */
	DetectorGLSurfaceView(Context context, boolean detect)
	{
		super(context);
		//setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		_trenderer = new DetectorRenderer(context, detect);
		
		setRenderer(_trenderer);
		setRenderMode(RENDERMODE_WHEN_DIRTY);
		setKeepScreenOn(true);
	}
	
}