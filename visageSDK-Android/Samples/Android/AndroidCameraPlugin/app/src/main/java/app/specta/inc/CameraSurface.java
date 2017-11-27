package app.specta.inc;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.SurfaceHolder;
import android.util.Log;

/**
 * Created by bogdan on 11/26/17.
 */

public class CameraSurface  extends GLSurfaceView  implements SurfaceHolder.Callback {

    CameraActivity  cameraRenderer;

    CameraSurface(CameraActivity context){
        super(context);
        cameraRenderer = context;
        setEGLContextClientVersion(2);
        setRenderer(cameraRenderer);

        Log.d("BOGDAN" , "########  CameraSurface created");
     setRenderMode(RENDERMODE_CONTINUOUSLY);
     setKeepScreenOn(true);
    }
}
