package ro.bgx.framerendererlib;

import android.util.Log;

public class FrameRendererPlugin {
    private static final FrameRendererPlugin ourInstance = new FrameRendererPlugin();

    private  static final String LOGTAG = "BGX-FRP";

    private long startTime;

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
