package com.visagetechnologies.visagetrackerdemo;

import android.content.Context;
import android.opengl.GLSurfaceView;

/** Implementation of GLSurfaceView class provided by Android.
 * 
 *  Creates and sets OpenGL renderer that contains the actual rendering code to render on this view.
 */
public class TrackerGLSurfaceView extends GLSurfaceView
{
	private TrackerRenderer _trenderer;
	
	/** Constructor.
	 * 
	 * @param context global information about android application environment.
	 */
	TrackerGLSurfaceView(Context context)
	{
		super(context);
		//setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		_trenderer = new TrackerRenderer(context);
		
		setRenderer(_trenderer);
		setRenderMode(RENDERMODE_CONTINUOUSLY);
		setKeepScreenOn(true);
	}
	
}
