package com.visagetechnologies.facedetectdemo;


import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.widget.RelativeLayout;

/** Activity called to initiate and control detection.
*/
public class DetectorActivity extends Activity
{
	int count = 0;
	int chooseFeature;
	
	public void onCreate(Bundle savedInstanceState)
	{
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    		
    	super.onCreate(savedInstanceState);    	
    	
        Bundle bundle = getIntent().getExtras();
        
        String path = bundle.getString("image");

		chooseFeature = bundle.getInt("type");
        
        //Send the image, initialise the detector and draw results
		StartDetector(path);
        
	}

	@Override
	public void onPause() {
		super.onPause();
		this.finish();
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		CleanUp();	
	}
	
	/** Called to initialize and start detection. Also initializes surface for displaying detection results.
     * 
     * @param path absolute path to image file used for detecting. Sent from ImagesActivity using Bundle class provided by Android. 
     */
	public void StartDetector(String path)
	{
		ImageDetectorView tpreview = new ImageDetectorView(this, path, true);
    	RelativeLayout layout = new RelativeLayout(this);
    	
    	layout.addView(tpreview);
    	
    	DetectorGLSurfaceView tGLView = new DetectorGLSurfaceView(this, true);
    	tpreview.setGLView(tGLView);
    	layout.addView(tGLView);
    	
    	RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    	
    	layout.setLayoutParams(params);
    	setContentView(layout);
    	
    	DetectorInit(getFilesDir().getAbsolutePath(), ".", chooseFeature);
	}
	
	/**Interface to native method used to initialize VisageFeaturesDetector object.
	 * 
	 * @param appPath path to root folder where Android application data is stored
	 * @param modelName name of generic Active Appearance Model file used for detection
	 */
	public native void DetectorInit(String appPath, String modelName, int chooseFeature);
	
	/**Interface to native method used to clean up resources.
	 * 
	 */
	public native void CleanUp();
	
	static
	{
		System.loadLibrary("VisageVision");
		System.loadLibrary("FaceDetectorWrapper");
	}
}
