package app.specta.inc.camera;


import android.content.res.Configuration;
import android.app.AlertDialog;
import android.graphics.Canvas;
import android.opengl.GLSurfaceView;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.graphics.ImageFormat;
import android.graphics.YuvImage;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.util.TypedValue;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.WindowManager;

import android.opengl.GLES20;

import java.io.IOException;
import java.util.List;
import java.lang.*;
import java.lang.String;

import com.google.android.gms.vision.Frame;
import com.unity3d.player.UnityPlayer;
import com.unity3d.player.UnityPlayerActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.google.android.gms.vision.CameraSource;
import com.google.android.gms.vision.Detector;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;
import com.google.android.gms.vision.MultiDetector;

import android.content.pm.PackageManager;
import android.util.SparseArray;
import android.Manifest;
import android.support.v4.app.ActivityCompat;
import java.nio.ByteBuffer;


public class CameraActivity extends UnityPlayerActivity implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    public final String TAG = "SPECTA-Camera3";
    private static final int REQUEST_CAMERA_PERMISSION = 201;
    Camera cam;
    int ImageWidth = -1;
    int ImageHeight = -1;
    SurfaceTexture tex;
    public float cameraFps;
    int pickCam = -1;
    int camId = -1;
    int orientation;
    boolean openCam = false;

    boolean surfaceReady = false;

    // unity texture
    private int nativeTexturePointer = -1;


    private boolean scannerEnabled = false;
    private BarcodeDetector barcodeDetector;
    private CameraSource cameraSource;


    /**
     * Called when device is rotated; calculates new screen orientation
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {

        super.onConfigurationChanged(newConfig);

        CameraInfo camInfo = new CameraInfo();

        try {
            Camera.getCameraInfo(camId, camInfo);
        } catch (Exception e) {

        }
        Display display = ((WindowManager) getSystemService(WINDOW_SERVICE)).getDefaultDisplay();

        orientation = camInfo.orientation;

        if (camInfo.facing == CameraInfo.CAMERA_FACING_FRONT)
            setParameters((display.getRotation() * 90 + orientation) % 360, -1, -1, -1);
        else if (camInfo.facing == CameraInfo.CAMERA_FACING_BACK)
            setParameters((orientation - display.getRotation() * 90 + 360) % 360, -1, -1, -1);
    }

    @Override
    public void onPause() {
        closeCamera();
        if (cameraSource != null) {
            cameraSource.release();
        }
        super.onPause();
    }

    @Override
    public void onResume() {
        if (openCam) {
            GrabFromCamera(ImageWidth, ImageHeight, pickCam);
        }

        if (scannerEnabled) {
            startScanner(3);
        }
        super.onResume();
    }

    /**
     * Calculates supported frame dimensions that are closest (by eucledian distance) to desired width and height
     *
     * @param parameters camera parameters
     * @param width      desired frame width
     * @param height     desired frame height
     */

    private void setPreviewSize(Camera.Parameters parameters, int width, int height) {
        int idx = 0;
        double dist1 = 100000;
        double dist2 = 100000;
        List<Size> sizes = parameters.getSupportedPreviewSizes();
        for (int i = 0; i < sizes.size(); i++) {
            dist2 = Math.abs(Math.sqrt(Math.pow((sizes.get(i).width - width), 2) + Math.pow((sizes.get(i).height - height), 2)));
            if (dist2 < dist1) {
                idx = i;
                dist1 = dist2;
            }
        }

        Log.i(TAG, "SettingPreview size to:" + width + "x" + height);
        parameters.setPreviewSize(sizes.get(idx).width, sizes.get(idx).height);
    }

    /**
     * Start grabbing frames from camera
     */
    public void GrabFromCamera(int imWidth, int imHeight, int pickCamera) {
        //on first call (from onResume), function parameters are set to -1; frame dimensions and camera are set on next call from Unity script
        if (imWidth == -1 || imHeight == -1) {
            imWidth = 320;
            imHeight = 240;
        }

        this.pickCam = pickCamera;
        camId = getCameraId(pickCam);
        try {
            cam = Camera.open(camId);
        } catch (Exception e) {
            Log.e(TAG, "Unable to open camera");
            return;
        }

        ImageWidth = imWidth;
        ImageHeight = imHeight;
        Camera.Parameters parameters = cam.getParameters();
        setPreviewSize(parameters, ImageWidth, ImageHeight);
        parameters.setPreviewFormat(ImageFormat.NV21);
        cam.setParameters(parameters);
        Display display = ((WindowManager) getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
        int screenOrientation = display.getRotation();

        final Size previewSize = cam.getParameters().getPreviewSize();
        Log.i(TAG, "Current preview size is " + previewSize.width + ", " + previewSize.height);

        int dataBufferSize = (int) (previewSize.height * previewSize.width *
                (ImageFormat.getBitsPerPixel(cam.getParameters().getPreviewFormat()) / 8.0));

        for (int i = 0; i < 10; i++) {
            cam.addCallbackBuffer(new byte[dataBufferSize]);
        }

        tex = new SurfaceTexture(0);
        try {
            cam.setPreviewTexture(tex);
        } catch (IOException e) {
            e.printStackTrace();
        }

        CameraInfo cameraInfo = new CameraInfo();
        Camera.getCameraInfo(camId, cameraInfo);
        orientation = cameraInfo.orientation;
        int flip = 0;
        if (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT) {
            flip = 1; // Mirror image from frontal camera
        }
        ImageWidth = previewSize.width;
        ImageHeight = previewSize.height;

        if (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT)
            setParameters((screenOrientation * 90 + orientation) % 360, ImageWidth, ImageHeight, flip);
        else
            setParameters((orientation - screenOrientation * 90 + 360) % 360, ImageWidth, ImageHeight, flip);

        cam.setPreviewCallbackWithBuffer(new PreviewCallback() {
            private long timestamp = 0;

            public void onPreviewFrame(byte[] data, Camera camera) {
                //Log.v("CameraTest","FPS = "+1000.0/(System.currentTimeMillis()-timestamp));
                //cameraFps = 1000.0f/(System.currentTimeMillis()-timestamp);
                //timestamp=System.currentTimeMillis();
                WriteFrame(data);
                camera.addCallbackBuffer(data);
            }
        });
        cam.startPreview();
        openCam = true;
    }

    // check for OpenGL errors
    private void checkGlError(String op) {
        int error;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.e(TAG, op + ": glError 0x" + Integer.toHexString(error));
        }
    }

    public void closeCamera() {
        if (cam != null) {
            cam.stopPreview();
            cam.release();
            cam = null;
        }
    }

    public int getPreviewSizeWidth() {
        return ImageWidth;
    }

    public int getPreviewSizeHeight() {

        return ImageHeight;
    }

    public int getNativeTexturePointer() {
        return nativeTexturePointer;
    }

    /*
     * JAVA texture creation
     */
    public int startCamera() {

        if (!surfaceReady) {
            Log.d(TAG, "#### StartCamera - surface not ready");
            return -1;
        }

        if (nativeTexturePointer > 0) {
            Log.d(TAG, "####### startCamera nativeTexturePointer=" + nativeTexturePointer);
            return nativeTexturePointer;
        }

        // open the camera
        cam = Camera.open();
        setupCamera();

        Log.d(TAG, "###### camera opened: " + (cam != null));

        try {
            cam.setPreviewTexture(tex);

            cam.setPreviewCallbackWithBuffer(new PreviewCallback() {
                private long timestamp = 0;

                public void onPreviewFrame(byte[] data, Camera camera) {
                    Log.v("CameraTest", "FPS = " + 1000.0 / (System.currentTimeMillis() - timestamp));
                    //cameraFps = 1000.0f/(System.currentTimeMillis()-timestamp);
                    timestamp = System.currentTimeMillis();
                    WriteFrame(data);
                    camera.addCallbackBuffer(data);
                    //updateTexture();
                    Log.v(TAG, "Camera preview ended :" +
                            data[11] + "|" + data[22] + "|" + data[33] + "|" + data[44] + "|" + data[55] + "|" + data[66] + "|" + data[77]);
                }
            });

            cam.startPreview();
            openCam = true;

        } catch (IOException ioe) {
            Log.w("MainActivity", " ##### CAM LAUNCH FAILED");
        }

        Log.d(TAG, "#######  CAMERA START END -- nativeTexturePointer=" + nativeTexturePointer);
        return nativeTexturePointer;
    }


    public int startScanner(int nativeCode) {


        Log.d(TAG, "#### Start QR scanner - v0.1  ---> closing the old camera preview");
        if (cam != null) {
            cam.stopPreview();
            cam.release();
            cam = null;
        }

        Log.d(TAG, "#### Start QR scanner - v0.1");

        scannerEnabled = true;
        barcodeDetector = new BarcodeDetector.Builder(this)
                .setBarcodeFormats(Barcode.QR_CODE)
                .build();

        if (barcodeDetector.isOperational()){
            Log.w(TAG, "mobile Vision detector dependencies are not yet available.");
        }

        Detector frameDetector = new Detector() {
            @Override
            public SparseArray detect(Frame frame) {

                ByteBuffer buffer = frame.getGrayscaleImageData();
                Log.w(TAG, "Frame Detector - ON DETECT --->" + buffer.remaining());
                int w = frame.getMetadata().getWidth();
                int h = frame.getMetadata().getHeight();

                byte[] arr = new byte[buffer.remaining()];
                buffer.get(arr);

                YuvImage yuvimage=new YuvImage(arr, ImageFormat.NV21, w, h, null);

                WriteFrame(yuvimage.getYuvData());
                return null;
            }
        };

        if(frameDetector.isOperational()) {
            Log.w(TAG, "mobile camera preview frame detector is operational");
        } else {
            Log.w(TAG, "mobile camera preview frame detector dependencies are not yet available.");
        }
        frameDetector.setProcessor(new Detector.Processor() {
            @Override
            public void release() {

            }

            @Override
            public void receiveDetections(Detector.Detections detections) {

            }
        });


        barcodeDetector.setProcessor(new Detector.Processor<Barcode>() {
            @Override
            public void release() {
                Log.w(TAG, "### To prevent memory leaks barcode scanner has been stopped");
            }

            @Override
            public void receiveDetections(Detector.Detections<Barcode> detections) {
                final SparseArray<Barcode> barcodes = detections.getDetectedItems();
                if (barcodes.size() != 0) {

                    String qrCode= barcodes.valueAt(0).displayValue;

                    Log.w(TAG, "### JAVA QR scanner detected:" + qrCode);
                    onCodeDetected(qrCode);

                }
            }
        });

        MultiDetector multiDetector = new MultiDetector.Builder()
                .add(frameDetector)
                .add(barcodeDetector)
                .build();

        cameraSource = new CameraSource.Builder(this, multiDetector)
                .setRequestedPreviewSize(480, 360).setRequestedFps(30)
                .setAutoFocusEnabled(true) //you should add this feature
                .build();

        try {
            if (ActivityCompat.checkSelfPermission(CameraActivity.this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
                cameraSource.start();
            } else {
                ActivityCompat.requestPermissions(CameraActivity.this, new
                        String[]{Manifest.permission.CAMERA}, REQUEST_CAMERA_PERMISSION);
            }

        } catch (IOException e) {
            e.printStackTrace();
        }


        return  1;
    }

    public void releaseScanner(int nativeCode){

        Log.w(TAG, "### Releasing QR scanner..");
        if(scannerEnabled){
            cameraSource.release();
        }
        scannerEnabled = false;

        Log.w(TAG, "### Starting the old camera preview");
        startCamera();

        Log.w(TAG, "### Preview started");
    }

    //@SuppressLint("NewApi")
    private void setupCamera() {
        Camera.Parameters parms = cam.getParameters();

        // Give the camera a hint that we're recording video. This can have a
        // big impact on frame rate.
        parms.setRecordingHint(true);
        parms.setPreviewFormat(ImageFormat.NV21);

        // leave the frame rate set to default
        cam.setParameters(parms);

        Camera.Size mCameraPreviewSize = parms.getPreviewSize();
        ImageWidth = parms.getPreviewSize().width;
        ImageHeight = parms.getPreviewSize().height;

        // only for debugging output
        int[] fpsRange = new int[2];
        parms.getPreviewFpsRange(fpsRange);
        String previewFacts = mCameraPreviewSize.width + "x"
                + mCameraPreviewSize.height;
        if (fpsRange[0] == fpsRange[1]) {
            previewFacts += " @" + (fpsRange[0] / 1000.0) + "fps";
        } else {
            previewFacts += " @[" + (fpsRange[0] / 1000.0) + " - "
                    + (fpsRange[1] / 1000.0) + "] fps";
        }

        Log.i(TAG, "previewFacts=" + previewFacts);

        checkGlError("endSetupCamera");
    }

    /**
     * Getting camera device ID
     *
     * @param imCamID if set to 1, back facing camera will be chosen; else front facing camera will be chosen;
     * @return camera device ID
     */

    int getCameraId(int pickCamera) {
        int cameraId = -1;
        int numberOfCameras = Camera.getNumberOfCameras();
        for (int i = 0; i < numberOfCameras; i++) {
            CameraInfo info = new CameraInfo();
            Camera.getCameraInfo(i, info);
            cameraId = i;

            Log.e(TAG, "getCameraId :" + i);
            if (info.facing == CameraInfo.CAMERA_FACING_FRONT && pickCamera != 1)
                return cameraId;
            else if (info.facing == CameraInfo.CAMERA_FACING_BACK && pickCamera == 1)
                return cameraId;
        }
        return -1;
    }

    public void AlertDialogFunction(final String message) {
        runOnUiThread(new Runnable() {
            private String licenseMessage = message;

            @Override
            public void run() {
                TextView title = new TextView(UnityPlayer.currentActivity);
                title.setText("License warning");
                title.setGravity(Gravity.CENTER);
                title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);

                TextView msg = new TextView(UnityPlayer.currentActivity);
                msg.setText(message);
                msg.setGravity(Gravity.CENTER);
                msg.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);

                new AlertDialog.Builder(UnityPlayer.currentActivity, AlertDialog.THEME_TRADITIONAL)
                        .setCustomTitle(title)
                        .setView(msg)
                        .setPositiveButton(android.R.string.yes, null)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .show();

            }
        });
    }

    int getCameraId() {
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

    @Override
    public void onDestroy() {
        closeCamera();

        super.onDestroy();
    }


    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        init();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        //resize(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        render();
    }

    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        // Log.i(TAG, "#####   onSurfaceCreated=");
        // updateTexture();
    }

    public static native void WriteFrame(byte[] frame);

    public static native void setParameters(int orientation, int width, int height, int flip);

    public static native void bindTexture(int pointer);

    public static native void init();

    public static native void resize(int width, int height);

    public static native void render();

    public static native void onCodeDetected(String code);

    static {
        System.loadLibrary("VisageVision");
        System.loadLibrary("VisageTrackerUnityPlugin");
    }


}