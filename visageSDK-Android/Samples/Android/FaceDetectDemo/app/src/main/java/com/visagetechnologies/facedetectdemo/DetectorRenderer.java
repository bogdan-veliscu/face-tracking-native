package com.visagetechnologies.facedetectdemo;

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
import android.view.Display;
import android.view.WindowManager;

/** TrackerRenderer class provides implementation of rendering functions required for displaying detection results.
 */
public class DetectorRenderer implements GLSurfaceView.Renderer
{	
	int _width;
	int _height;
	int[] textures = new int[3];
	Context _context;
	boolean _detect;

	/** Constructor.
	 * 
	 * @param context global information about android application environment.
	 */
	DetectorRenderer(Context context, boolean detect)
	{	
		_context = context;
		_detect = detect;
	}
	
	/** Implementation of onDrawFrame method provided by GLSurfaceView.Renderer interface.
	 *  Called to render every frame.
	 *  
	 *  @param gl OpenGL context.
	 */
	public void onDrawFrame(GL10 gl) 
	{
		gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
		gl.glEnable(GL10.GL_BLEND);

		gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
		if (_detect)
			displayResults();
		else
			displayIdentity();
		drawFrame(_width, _height);
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

		gl.glGenTextures(3, textures, 0);
	}


	/** Interface to native method used to draw detection results.
	 *
	 */
	public native void displayResults();

	/** Interface to native method used to draw recognition results.
	 *
	 */
	public native void  displayIdentity();

	/** Interface to native method used to draw current frame analyzed by detector.
	 *
	 */
	public native void drawFrame(int _width, int _height);

	static
	{
		System.loadLibrary("VisageVision");
		System.loadLibrary("FaceDetectorWrapper");
	}
}
