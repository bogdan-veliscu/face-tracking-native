package com.visagetechnologies.visagetrackerdemo;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.view.WindowManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;


/** Activity started when user interacts with Track from image button in StartupActivity.
 * ImagesActivity images gallery on Android device. User can choose image that will be used for tracking.
 */
public class ImagesActivity extends Activity
{
	private static int RESULT_LOAD_IMAGE = 1;
	
	/** Implementation of onCreate method provided by Activity interface.
	 *  Called when Activity is started for the first time.
	 */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        Intent i = new Intent(
				Intent.ACTION_PICK,
				android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
		
		startActivityForResult(i, RESULT_LOAD_IMAGE);

    }
    
    /** Called when user selects image.
	 * 
	 */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
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
			
			Intent intent = new Intent(this, TrackerActivity.class);
	    	
	    	Bundle bundle = new Bundle();
	    	bundle.putString("image", picturePath);
	    	bundle.putInt("type", 1);
	    	
	    	intent.putExtras(bundle);
	    	startActivity(intent);
		}
		finish();
    
    }
}
