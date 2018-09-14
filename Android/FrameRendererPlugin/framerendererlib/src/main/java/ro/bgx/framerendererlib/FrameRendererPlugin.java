package ro.bgx.framerendererlib;

import android.util.Log;

public class FrameRendererPlugin {
    private static final FrameRendererPlugin ourInstance = new FrameRendererPlugin();

    private  static final String LOGTAG = "BGX-FRP";

    private long startTime;

    static {
        Log.w("BGX", "before loading native library");
        System.loadLibrary("native-framerenderer");
        Log.w("BGX", "before loading native library");
    }

    public static native void draw(byte[] data, int width, int height, int rotation);

    public static native void setScannerEnabled(int enabled);



    public static FrameRendererPlugin getInstance() {
        return ourInstance;
    }

    private FrameRendererPlugin() {

        Log.w(LOGTAG, "# Created FrameRenderer plugin");

    }

    public double getElapsedTime(){
        return (System.currentTimeMillis()-startTime)/ 1000.0f;
    }
}
