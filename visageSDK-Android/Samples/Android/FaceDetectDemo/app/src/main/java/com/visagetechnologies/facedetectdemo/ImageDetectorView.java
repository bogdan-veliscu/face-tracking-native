package com.visagetechnologies.facedetectdemo;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.view.SurfaceHolder;

/** Derives from TrackerView. 
 * Class ImageTrackerView is used for providing detector with necessary image data and controlling surface used to display results of detection.
 * Implements Runnable interface provided by Android. Runnable interface is used for implementation of simple multi-threading.    
 */
public class ImageDetectorView extends DetectorView implements Runnable
{
	/**
	 *  Bitmap of image used for tracking.
	 */
	Bitmap _bitmap;
	
	/**
	 * Absolute path to image used for tracking.
	 */
	String _path;
	Context _context;
	boolean _isDetected;
	boolean _detect;
	
	/** Constructor. 
	 * 
	 * @param context global information about Android application environment.
	 * @param path absolute path to image file used for tracking. Sent from DetectorActivity.
	 */
	ImageDetectorView(Context context, String path, boolean detect)
	{
		super(context);
		_context = context;
		_path = path;
		_detect = detect;
		_isDetected = false;
		LoadFromFile();
		getHolder().addCallback(this);
	}
	
	/** Implementation of surfaceCreated method provided by SurfaceHolder.Callback class.
	 *  Creates and starts rendering thread. 
	 *  Calls DetectFeatures method to pass image data to detector.
	 */
	public void surfaceCreated(SurfaceHolder holder)
	{	
    	setKeepScreenOn(true);
		SetOptimalBitmapSize();
		if (_detect)
			DetectFeatures(ConvertToByte(), _bitmap.getWidth(), _bitmap.getHeight());
		else
			recognizeFace(ConvertToByte(), _bitmap.getWidth(), _bitmap.getHeight());
	}
	
	/** Implementation of surfaceChanged method provided by SurfaceHolder.Callback class.
	 * Sets new width and height values of view.
	 */
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
	{
		_width = width;
		_height = height;
	}
	
	/** Implementation of surfaceDestroyed method provided by SurfaceHolder.Callback class.
	 *
	 */
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		
	}
	
	/** Called to obtain width of bitmap used for tracking.
	 * 
	 * @return width of bitmap in pixels
	 */
	 public int getBitmapWidth()
	 {
		 return _bitmap.getWidth();
	 }
	 
	 /** Called to obtain height of bitmap used for tracking.
	  * 
	  * @return height of bitmap in pixels.
	  */
	 public int getBitmapHeight() 
	 {
		 return _bitmap.getHeight();
	 }
	 	 
	 /** Utility method used to resize bitmap of image used for tracking. 
	  * Images that are larger than the screen are resized to fit the screen while keeping the initial ratio between height and width.
	  * In order to interpret images in a correct way OpenCV requires image size to be power of two.
	  * Images that have either width or height value that does not satisfy this condition are additionally resized until they do. This slightly changes ratio between width and height.
	  */
	 public void SetOptimalBitmapSize()
	 {
		 int maxW = _context.getResources().getDisplayMetrics().widthPixels;
		 int maxH =  _context.getResources().getDisplayMetrics().heightPixels;
		 
		 int w = maxW;
		 int h = maxH;
			
		 if(_bitmap.getWidth() < maxW && _bitmap.getHeight() < maxH && _bitmap.getWidth() % 4 == 0 && _bitmap.getHeight() % 4 == 0)
		 {
			 return;
		 }
		 
		 int bitmapW = _bitmap.getWidth();
		 int bitmapH = _bitmap.getHeight();
		 
		 if(_bitmap.getWidth() > _bitmap.getHeight())
		 { 
			 float ratio = (float)bitmapH / (float)bitmapW; 
			 while(true)
			 {
				 if(w < maxW) 
				 {
					 break;
				 }			 
				 w = w / 2;
			 }
			 h = Math.round(ratio * w);
		 }
		 
		 if(_bitmap.getWidth() < _bitmap.getHeight())
		 {
			 float ratio = (float)bitmapW / (float)bitmapH; 
			 while(true)
			 {
				if(h < maxH) 
			 	{
				 	break;
			 	}			 
			 	h = h / 2;
			 }
			 w = Math.round(ratio * h);
		 }
		 if(_bitmap.getWidth() == _bitmap.getHeight())
		 {
			 float ratio = (float)bitmapW / (float)bitmapH;
			 while(true)
			 {
				if(h < maxH) 
			 	{
				 	break;
			 	}			 
			 	h = h / 2;
			 }
			w = Math.round(ratio * h);
		 }
		if(w % 4 != 0)
		{
			while(true)
			{
				w++;
				if(w % 4 == 0)
				{
					break;
				}
			}
		}
		
		if(h % 4 != 0)
		{
			while(true)
			{
				h++;
				if(h % 4 == 0)
				{
					break;
				}
			}
		}
		_bitmap = Bitmap.createScaledBitmap(_bitmap, w, h, true);
	 }
	
	/** Utility method called for loading the image from file into a Bitmap object.
	 */
	public void LoadFromFile()
	{
		File image = new File(_path);
		
		FileInputStream fis = null;
		try 
		{
			fis = new FileInputStream(image) ;
		} 
		catch (FileNotFoundException e) 
		{
			e.printStackTrace();
		}
		_bitmap = BitmapFactory.decodeStream(fis);	
	}
	
	/** Utility method called to obtain raw pixel data from bitmap object in which image used for detection is loaded. 
	 * Pixel data is obtained as array of integers by calling getPixels method of Bitmap class.
	 * Each element of obtained array of integer represents one pixel of bitmap. 
	 * In order to create pixel data usable by tracker it is necessary to extract red, green and blue color values from array of integers and store it as a byte array. 
	 * 
	 * @return byte array of raw pixel data obtained from image used for detection.
	 */
	public byte[] ConvertToByte()
	{
		int w = _bitmap.getWidth();
		int h = _bitmap.getHeight();
		
		int[] rgbStream = new int[ w * h];
		_bitmap.getPixels(rgbStream, 0, w, 0, 0, w, h);
		
		byte[] buffer = new byte [w * h * 4];
		int offset = 0;
		for(int i = 0; i < w * h; i++)
		{
			int pixel = rgbStream[i];
			//int A = (pixel >> 24) & 0xFF;
			int R = (pixel >> 16) & 0xFF;
			int G = (pixel >> 8) & 0xFF;
			int B = pixel & 0xFF;
			
			buffer[offset + 0] = (byte)R;
			buffer[offset + 1] = (byte)G;
			buffer[offset + 2] = (byte)B;
			
			offset = offset + 3;			
		}
		
		return buffer;
	}
	
	/** Implementation of run method of Runnable interface. 
	 * Android requires that all rendering to a surface is done from separate thread than the one which created the View object. Rendering thread is started from surfaceCreated method.
	 * Method calls rendering logic of application.
	 */
	public void run()
	{
		SurfaceHolder surfaceHolder = getHolder();
		Canvas c = null;
    	isRunning = true;
			try
			{
				//int st = GetStatus();
				//int count = GetNotifyCount();
				c = surfaceHolder.lockCanvas(null);		
				synchronized(this)
				{	

					_tview.requestRender();							
				}			
			}
			finally
			{
				if(c != null)
				{
					surfaceHolder.unlockCanvasAndPost(c);
				}
			}	
		return;
	}
	
	/** Interface to native method used for passing raw pixel data to detector. Called from surfaceCreated.
	 * 
	 * @param frame raw pixel data of image used for tracking.
	 * @param width width of image used for tracking.
	 * @param height height of image used for tracking.
	 */
	public static native void DetectFeatures(byte[] frame, int width, int height);
	public static native void recognizeFace(byte[] frame, int width, int height);
}
