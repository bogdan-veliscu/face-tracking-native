package com.visagetechnologies.visagetrackerdemo;

import java.lang.ref.WeakReference;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.os.Bundle;
import android.os.Handler;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.util.TypedValue;
import android.view.Display;
import android.view.Gravity;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;



/** Activity called to initiate and control tracking for either tracking from camera or image.
 */
public class TrackerActivity extends Activity 
{
	final int TRACK_GREYSCALE = 2;
	final int TRACK_RGB = 0;

	final int VISAGE_CAMERA_UP = 0;
	final int VISAGE_CAMERA_DOWN = 1;
	final int VISAGE_CAMERA_LEFT = 2;
	final int VISAGE_CAMERA_RIGHT = 3;

	private Handler mHandler = new Handler();
	private TextView tv;
	private static boolean licenseMessageShown = false;

	public static float AverageFPS = 0;
	public static int FPSCounter = 0;
	public static TrackerActivity instance;

	public static boolean wait = false;
	JavaCamTrackerView cpreview;
	ImageTrackerView ipreview;

	int cameraId = -1;
	int orientation;
	int type;

	@Override
    public void onCreate(Bundle savedInstanceState) {
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		super.onCreate(savedInstanceState);
		Bundle bundle = getIntent().getExtras();
		
		type = bundle.getInt("type");
        switch(type)
        {
	    	case 0:
	    	{
			StartCam();
			break;
		}
        	case 1:
        	{
			String path = bundle.getString("image");
			StartImage(path);
			break;
		}
		}
		TrackerActivity.instance = this;
		licenseMessageShown = false;
		AverageFPS = 0;
		FPSCounter = 0;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		if (cpreview != null && type == 0)
			cpreview.closeCamera();
		TrackerStop();
		mHandler.removeCallbacksAndMessages(null);
	}
	
	
	@Override
	public void onPause() {
		super.onPause();
		this.finish();
	}
	
	/**
	 * Thread for tracking from camera
	 */
	
	private final class CameraThread implements Runnable {
		private int Cwidth;
		private int Cheight;
		private int Corientation;
		private int Cflip;

		protected CameraThread(int width, int height, int flip, int orientation) {

			Cwidth = width;
			Cheight = height;
			Corientation = orientation;
			Cflip = flip;
		}

		@Override
		public void run() {
			setParameters(Cwidth, Cheight, Corientation, Cflip);
			TrackFromCam();
			return;
		}
	}
	
	/**
	 * Thread for tracking from image
	 */
	
	private final class ImageThread implements Runnable {
			
			private int Iwidth;
			private int Iheight;
			
		protected ImageThread(int width, int height) {
			Iwidth = width;
			Iheight = height;
		}

		@Override
		public void run() {
				TrackFromImage(Iwidth, Iheight);
		}
	}

	private static final class DoneRunnable implements Runnable {
		private final WeakReference<TextView> mTextViewRef;
		private final WeakReference<Handler> mHandlerRef;

		protected DoneRunnable(TextView textView, Handler handler) {
			mTextViewRef = new WeakReference<TextView>(textView);
			mHandlerRef = new WeakReference<Handler>(handler);
		}
		@Override
		public void run() {
			final TextView tv = mTextViewRef.get();
			if (tv!=null)
			{
				float fps = GetFps();
				float displayFps = GetDisplayFps();
				String state = GetStatus();

				int trackTime = GetTrackTime();

				if (!state.equals("OFF")) {
					if (TrackerActivity.AverageFPS == 0) {
						if (fps > 0 && fps < 100) {
							TrackerActivity.AverageFPS = fps;
							TrackerActivity.FPSCounter++;
						}
					}
					else {
						if (fps > 0 && fps < 80) {
							TrackerActivity.FPSCounter++;
							TrackerActivity.AverageFPS = TrackerActivity.AverageFPS + ((fps - TrackerActivity.AverageFPS) / TrackerActivity.FPSCounter);
						}

					}

					tv.setText("FPS: " + String.format("%.2f", fps) + " (track " + String.format("%d", trackTime) + " ms)" + "\nDISPLAY FPS: " + String.format("%.2f", displayFps) + "\nStatus: " + state + "\nAvgFPS: " + String.format("%.2f", TrackerActivity.AverageFPS));
				} else if (state.equals("OFF") && FPSCounter > 100 && licenseMessageShown == false) {
					new AlertDialog.Builder(TrackerActivity.instance)
							.setTitle("Warning")
						.setMessage("You are using an unlicensed copy of visage|SDK\nTracking time is limited to one minute.\nPlease contact Visage Technologies to obtain a license key.\n30-day trial licenses are available.\nTracking will now stop.")
						.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
					        public void onClick(DialogInterface dialog, int which) { 
										}
									})
					    .setIcon(android.R.drawable.ic_dialog_alert)
					    .show();
					tv.setText("");
					licenseMessageShown = true;
				}
			}
			final Handler handler = mHandlerRef.get();
			if (handler != null)
				handler.postDelayed(this, 200);
		}
	}

	/**
	 * Starts tracking from camera and initializes surface for displaying tracking results.
	 */
	public void StartCam(){		
		TrackerInit(getFilesDir().getAbsolutePath() + "/Facial Features Tracker - High.cfg", getFilesDir().getAbsolutePath());
		cameraId = getCameraId();
		cpreview = new JavaCamTrackerView(this, this, cameraId);
    	RelativeLayout layout = new RelativeLayout(this);
    	layout.addView(cpreview);
    	
    	TrackerGLSurfaceView tGLView = new TrackerGLSurfaceView(this);
    	cpreview.setGLView(tGLView);
    	layout.addView(tGLView);
    	
    	final TextView tv = new TextView(this);
    	tv.setTextColor(Color.GREEN);
    	tv.setBackgroundColor(Color.BLACK);
    	layout.addView(tv);
    	mHandler.postDelayed(new DoneRunnable(tv, mHandler), 200);
    	
    	RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    	
    	Display display = ((WindowManager) getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
    	int screenOrientation = display.getRotation();
    	
    	layout.setLayoutParams(params);
    	setContentView(layout);    	
    	CameraInfo cameraInfo = new CameraInfo();
    	Camera.getCameraInfo(cameraId, cameraInfo);
		int orientation = cameraInfo.orientation;
    	int flip = 0;		
		if (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT){
			flip = 1; // Mirror image from frontal camera
		}
		Thread CamThread;
		if (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT)
			CamThread = new Thread(new CameraThread(cpreview.previewWidth, cpreview.previewHeight, flip, (screenOrientation*90 + orientation)%360));
    	else
    		CamThread = new Thread(new CameraThread(cpreview.previewWidth, cpreview.previewHeight, flip, (orientation - screenOrientation*90 + 360)%360));
		
    	CamThread.start();

	}

	int getCameraId(){
		int cameraId = -1;
		int numberOfCameras = Camera.getNumberOfCameras();
		for (int i = 0; i < numberOfCameras; i++) {
			CameraInfo info = new CameraInfo();
			Camera.getCameraInfo(i, info);
			cameraId = i;
	    	if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
	    		break;
		}
	}
		return cameraId;
	}
	  
	
    /** Called to initialize and start tracking for image. Also initializes surface for displaying tracking results.
     * 
     * @param path absolute path to image file used for tracking. Sent from ImagesActivity using Bundle class provided by Android. 
     */
    public void StartImage(String path)
    {
    	TrackerInit(getFilesDir().getAbsolutePath() + "/Facial Features Tracker - High.cfg", getFilesDir().getAbsolutePath());
		ipreview = new ImageTrackerView(this, path, this);
		RelativeLayout layout = new RelativeLayout(this);

		layout.addView(ipreview);

		TrackerGLSurfaceView tGLView = new TrackerGLSurfaceView(this);
		ipreview.setGLView(tGLView);
		layout.addView(tGLView);

		tv = new TextView(this);
		tv.setTextColor(Color.GREEN);
		tv.setBackgroundColor(Color.BLACK);
		layout.addView(tv);

		mHandler.postDelayed(new DoneRunnable(tv, mHandler), 200);

    	RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);

		layout.setLayoutParams(params);
		setContentView(layout);
    	
		ipreview.SetOptimalBitmapSize();

		Thread ImageThread = new Thread(new ImageThread(ipreview.getBitmapWidth(), ipreview.getBitmapHeight()));
		ImageThread.start();
	}

    public void ShowDialog()
    {
		Intent intent = new Intent(this, WarningActivity.class);
		startActivity(intent);
		this.finish();
	}
    
    
    /* 
     * Method that is called if the product is unlicensed
     */
    
    public void AlertDialogFunction(String message)
    {
    	TextView title = new TextView(this);
		title.setText("License warning");
		title.setGravity(Gravity.CENTER);
		title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);

		TextView msg = new TextView(this);
		msg.setText(message);
		msg.setGravity(Gravity.CENTER);
		msg.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
		
		new AlertDialog.Builder(this)
	    .setCustomTitle(title)
	    .setView(msg)
	    .setPositiveButton(android.R.string.yes, null)
	     .setIcon(android.R.drawable.ic_dialog_alert)
	     .show();
    }
    

	/** Interface to native method called for initializing tracker.
	 * 
	 * @param configFilename absolute path to tracker configuration file. 
	 */
	public native void TrackerInit(String configFilename, String appPath);

    /** Interface to native method called to start tracking from image.
	 * 
     * @param width width of image file used for tracking
     * @param height height of image used for tracking
	 */
	public native void TrackFromImage(int width, int height);

	/**
	 * Prepare raw image interface to track from camera.
	 */
    public native void TrackFromCam();

	/** Interface to native method called to obtain frame rate information from tracker.
	 * 
	 * @return frame rate value
	 */
	public static native float GetFps();

	/** Interface to native method called to obtain rendering frame rate information.
	 * 
	 * @return rendering frame rate value
	 */
	public static native float GetDisplayFps();

	/** Interface to native method called to obtain status of tracking information from tracker.
	 * 
	 * @return status of tracking information as text.
	 */
	public static native String GetStatus();

	/** Interface to native method called to obtain pure tracking time.
	 *
	 * @return status of tracking information as text.
	 */
	public static native int GetTrackTime();
	
	/**
	 * Interface to native method called to set camera frame parameteres
	 * 
	 */

	public static native void setParameters(int width, int height,
			int orientation, int flip);
	
	/** Interface to native method used to stop tracking.
	 */
	public native void TrackerStop();
	

	static {
		System.loadLibrary("VisageVision");
		System.loadLibrary("VisageWrapper");
	}
}