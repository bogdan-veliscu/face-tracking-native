package com.visagetechnologies.facedetectdemo;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.widget.Button;


/** Activity started when application is launched.
 * ImagesActivity images gallery on Android device. User can choose image that will be used for detection.
 */
public class ImagesActivity extends Activity
{
	private static int RESULT_LOAD_IMAGE = 1;
	private boolean doRestart = false;
	final int MY_PERMISSIONS_REQUEST_STORAGE = 1;

	boolean recognition = false;
	boolean detection = false;
	boolean analysis = false;

	Intent intentRecognition;
	Intent intentAnalysis;

	/** Implementation of onCreate method provided by Activity interface.
	 *  Called when Activity is started for the first time.
	 */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        copyAssets(getFilesDir().getAbsolutePath());
		setContentView(R.layout.main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

		Button buttonDetectClick = (Button)findViewById(R.id.btnDetect);
		buttonDetectClick.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				recognition = false; detection = true; analysis = false;
				handlePermissions();
			}
		});

		Button buttonRecognitionClick = (Button)findViewById(R.id.btnRecognize);
		buttonRecognitionClick.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				intentRecognition = new Intent(v.getContext(), RecognitionActivity.class);
				recognition = true; detection = false; analysis = false;
				handlePermissions();
			}
		});

		Button buttonAnalysisClick = (Button)findViewById(R.id.btnAnalyse);
		buttonAnalysisClick.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				recognition = false; detection = false; analysis = true;
				handlePermissions();
			}
		});
    }

	/** Method that handles permissions.
	 */
	public void handlePermissions() {
		if (ContextCompat.checkSelfPermission(this,
				Manifest.permission.WRITE_EXTERNAL_STORAGE)
				!= PackageManager.PERMISSION_GRANTED) {

			ActivityCompat.requestPermissions(this,
					new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
					MY_PERMISSIONS_REQUEST_STORAGE);
		}
		else
		{
			if (recognition == true)
				StartRecognition();
			else if (detection == true)
				StartDetection();
			else
				StartAnalysis();
		}
	}
	/** Method invoked when the user responds to permission request.
	 */
	@Override
	public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
		switch (requestCode) {
			case MY_PERMISSIONS_REQUEST_STORAGE: {
				// If request is cancelled, the result arrays are empty.
				if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

					if (recognition == true)
						StartRecognition();
					else if (detection == true)
						StartDetection();
					else
						StartAnalysis();
				}
				return;
			}
		}
	}

	public void StartDetection()
	{
		Intent i = new Intent(Intent.ACTION_PICK, android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
		startActivityForResult(i, RESULT_LOAD_IMAGE);
	}

	public void StartRecognition()
	{
		Bundle bundle = new Bundle();
		bundle.putString("image", "/data/data/com.visagetechnologies.facedetectdemo/files/Famous Actors/match/AngelinaJolie.jpg");
		bundle.putInt("type", 1);

		intentRecognition.putExtras(bundle);

		startActivity(intentRecognition);
	}

	public void StartAnalysis()
	{
		Intent i = new Intent(Intent.ACTION_PICK, android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
		startActivityForResult(i, RESULT_LOAD_IMAGE);
	}
    
    /** Called when user selects image.
	 * 
	 */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) 
    {
    	super.onActivityResult(requestCode, resultCode, data);
    	
		if (requestCode == RESULT_LOAD_IMAGE && resultCode == RESULT_OK && null != data) {
			Uri selectedImage = data.getData();
			String[] filePathColumn = { MediaStore.Images.Media.DATA };

			Cursor cursor = getContentResolver().query(selectedImage,
					filePathColumn, null, null, null);
			cursor.moveToFirst();

			int columnIndex = cursor.getColumnIndex(filePathColumn[0]);
			String picturePath = cursor.getString(columnIndex);
			cursor.close();
			
			Intent intent = new Intent(this, DetectorActivity.class);
	    	
	    	Bundle bundle = new Bundle();
	    	bundle.putString("image", picturePath);
			if (detection)
	    		bundle.putInt("type", 0);
			else
				bundle.putInt("type", 1);
	    	
	    	intent.putExtras(bundle);
	    	startActivity(intent);	
		
		} 
		else
		{
			finish();
		}
    
    }
    
    /** Utility method called to copy required file to trackerdata folder.
     * 
     * @param rootDir absolute path to directory where files should be copied. 
     * @param filename name of file that will be copied.
     */
public void copyFile(String rootDir, String filename) {
    AssetManager assetManager = this.getAssets();

    InputStream in = null;
    OutputStream out = null;
    try 
    {
        in = assetManager.open("trackerdata/" + filename);
        String newFileName = rootDir + "/" + filename;
        File file = new File(newFileName);
	    if(!file.exists()) 
	    {
	    	out = new FileOutputStream(newFileName);

	    	byte[] buffer = new byte[1024];
	    	int read;
	    	while ((read = in.read(buffer)) != -1) 
	    	{
	    		out.write(buffer, 0, read);
	    	}
	        out.flush();
	        out.close();
	        out = null;
	    }
	    in.close();
        in = null;
    } 
    catch (Exception e) 
    {
        Log.e("TrackerDemo", e.getMessage());
    }
}

/** Utility method called to create required directories and initiate copy of all assets required for tracking.
 * 
 * @param rootDir absolute path to root directory used for storing assets required for tracking.
 */
public void copyAssets(String rootDir) {
	
	AssetManager assetManager = this.getAssets();
	
	String assets[] = null;
	try {
		assets = assetManager.list("trackerdata");
		
		for (String asset : assets) {
			Log.i("FaceDetectDemo", rootDir + File.separator + asset);
			try {
				if (!asset.contains("bdtsdata")) copyFile(rootDir, asset);
			} catch (Exception e) {
				Log.e("FaceDetectDemo", e.getMessage());
			}
		}
	} catch (Exception e) {
		Log.e("FaceDetectDemo", e.getMessage());
	}

	// create dirs
	final String[] dirs = {
			"Famous Actors",
			"Famous Actors/gallery",
			"Famous Actors/match",
			"bdtsdata",
			"bdtsdata/FF",
			"bdtsdata/LBF",
			"bdtsdata/NN",
			"bdtsdata/LBF/pe",
			"bdtsdata/LBF/vfadata",
			"bdtsdata/LBF/ye",
			"bdtsdata/LBF/vfadata/ad",
			"bdtsdata/LBF/vfadata/ed",
			"bdtsdata/LBF/vfadata/gd"
	};
	
	for (String dirname : dirs) {
		try {
			File dir = new File(rootDir + File.separator + dirname);
			if (!dir.exists()) dir.mkdir();
		} catch (Exception e) {
			Log.e("FaceDetectDemo", e.getMessage());
		}
	}
	
	// copy files
	final String[] files = {
			"Famous Actors/gallery/AngelinaJolie.jpg",
			"Famous Actors/gallery/AudreyHepburn.jpg",
			"Famous Actors/gallery/BruceLee.jpg",
			"Famous Actors/gallery/MattDamon.jpg",
			"Famous Actors/gallery/RobertDeNiro.jpg",
			"Famous Actors/gallery/ScarlettJohansson.jpg",
			"Famous Actors/match/AngelinaJolie.jpg",
			"Famous Actors/match/AudreyHepburn.jpg",
			"Famous Actors/match/BruceLee.jpg",
			"Famous Actors/match/MattDamon.jpg",
			"Famous Actors/match/RobertDeNiro.jpg",
			"Famous Actors/match/ScarlettJohansson.jpg",
			"bdtsdata/FF/ff.dat",
			"bdtsdata/LBF/lv",
			"bdtsdata/LBF/pr.lbf",
			"bdtsdata/LBF/pe/landmarks.txt",
			"bdtsdata/LBF/pe/lp11.bdf",
			"bdtsdata/LBF/pe/W",
			"bdtsdata/LBF/vfadata/ad/ad0.lbf",
			"bdtsdata/LBF/vfadata/ad/ad1.lbf",
			"bdtsdata/LBF/vfadata/ad/ad2.lbf",
			"bdtsdata/LBF/vfadata/ad/ad3.lbf",
			"bdtsdata/LBF/vfadata/ad/ad4.lbf",
			"bdtsdata/LBF/vfadata/ad/regressor.lbf",
			"bdtsdata/LBF/vfadata/ed/ed0.lbf",
			"bdtsdata/LBF/vfadata/ed/ed1.lbf",
			"bdtsdata/LBF/vfadata/ed/ed2.lbf",
			"bdtsdata/LBF/vfadata/ed/ed3.lbf",
			"bdtsdata/LBF/vfadata/ed/ed4.lbf",
			"bdtsdata/LBF/vfadata/ed/ed5.lbf",
			"bdtsdata/LBF/vfadata/ed/ed6.lbf",
			"bdtsdata/LBF/vfadata/gd/gd.lbf",
			"bdtsdata/LBF/ye/landmarks.txt",
			"bdtsdata/LBF/ye/lp11.bdf",
			"bdtsdata/LBF/ye/W",
			"bdtsdata/NN/fa.lbf",
			"bdtsdata/NN/fc.lbf",
			"bdtsdata/NN/model.bin"
	};
	
	for (String filename : files) {
		try {
			Log.i("FaceDetectDemo", rootDir + File.separator + filename);
			copyFile(rootDir, filename);
		} catch(Exception e) {
			Log.e("FaceDetectDemo", e.getMessage());
		}
	}
}
	
}
