package com.visagetechnologies.visagetrackerdemo;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;
import javax.microedition.khronos.opengles.GL11Ext;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

/** TrackerRenderer class provides implementation of rendering functions required for displaying tracking results.
 */
public class TrackerRenderer implements GLSurfaceView.Renderer
{	
	int _width;
	int _height;
	int[] textures = new int[3];
	Context _context;
	
	/** Constructor.
	 * 
	 * @param context global information about android application environment.
	 */
	TrackerRenderer(Context context) 
	{	
		_context = context;
	}
	
	/** Implementation of onDrawFrame method provided by GLSurfaceView.Renderer interface.
	 *  Called to render every frame.
	 *  
	 *  @param gl OpenGL context.
	 */
	public void onDrawFrame(GL10 gl) 
	{	
		// gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

		boolean status = displayTrackingStatus(_width, _height);

		gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
		gl.glEnable(GL10.GL_BLEND);
	}
	
	/** Implementation of surfaceChanged method provided by SurfaceHolder.Callback class.
	 * Sets new width and height values of view.
	 */
	public void onSurfaceChanged(GL10 gl, int width, int height) 
	{
		_width = width;
		_height = height;
		
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) 
	{
		WindowManager wm = (WindowManager) _context.getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		Point size = new Point();
		display.getSize(size);
	}


	/** Interface to native method used to draw current frame analyzed by tracker and face model generated from tracking results 
	 * 
	 * @param width width of drawing view.
	 * @param height height of drawing view.
	 */
	public native boolean displayTrackingStatus(int width, int height);
	
	public native void DisplayInstructions();
	
	static
	{
		System.loadLibrary("VisageVision");
		System.loadLibrary("VisageWrapper");
	}
}
