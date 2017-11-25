package com.visagetechnologies.facedetectdemo;

import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.widget.RelativeLayout;

public class RecognitionActivity extends Activity {

    private AssetManager mgr;

    public void onCreate(Bundle savedInstanceState)
    {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        super.onCreate(savedInstanceState);

        Bundle bundle = getIntent().getExtras();

        String path = bundle.getString("image");

        mgr = getResources().getAssets();
        load(mgr);

        StartRecognition(path);
    }

    @Override
    public void onPause() {
        super.onPause();
        this.finish();
    }

    public void onDestroy()
    {
        super.onDestroy();
        CleanUp();
    }

    public void StartRecognition(String path)
    {
        ImageDetectorView tpreview = new ImageDetectorView(this, path, false);
        RelativeLayout layout = new RelativeLayout(this);

        layout.addView(tpreview);

        DetectorGLSurfaceView tGLView = new DetectorGLSurfaceView(this, false);
        tpreview.setGLView(tGLView);
        layout.addView(tGLView);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);

        layout.setLayoutParams(params);
        setContentView(layout);

        initRecognition(getFilesDir().getAbsolutePath());
    }

    private static native void load(AssetManager mgr);
    public native void initRecognition(String appPath);
    public native void CleanUp();

    static
    {
        System.loadLibrary("VisageVision");
        System.loadLibrary("FaceDetectorWrapper");
    }

}
