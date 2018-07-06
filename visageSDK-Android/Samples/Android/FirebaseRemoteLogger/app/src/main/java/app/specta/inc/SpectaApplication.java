package app.specta.inc;

import android.app.Activity;
import android.app.Application;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.firebase.storage.FirebaseStorage;
import com.google.firebase.storage.StorageReference;
import com.google.firebase.storage.UploadTask;

import java.io.File;
import java.io.IOException;
import java.net.URL;

public class SpectaApplication extends Application {

    public final String TAG = "SpectaAPP";
    File logFile;
    /**
     * Called when the application is starting, before any activity, service, or receiver objects (excluding content providers) have been created.
     */
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "### onCreate ");

        if ( isExternalStorageWritable() ) {

            File appDirectory = new File( Environment.getExternalStorageDirectory() + "/specta" );
            File logDirectory = new File( appDirectory + "/log" );
            logFile = new File( logDirectory, "logcat" + System.currentTimeMillis() + ".txt" );

            // create app folder
            if ( !appDirectory.exists() ) {
                appDirectory.mkdir();
            }

            // create log folder
            if ( !logDirectory.exists() ) {
                logDirectory.mkdir();
            }

            // clear the previous logcat and then write the new one to the file
            try {
                Process process = Runtime.getRuntime().exec("logcat -c");
                process = Runtime.getRuntime().exec("logcat -f " + logFile);
            } catch ( IOException e ) {
                e.printStackTrace();
            }

        } else {
            // not accessible
            Log.i(TAG, "### SD card not accessible");
        }

        this.registerActivityLifecycleCallbacks(new ActivityLifecycleCallbacks() {
            @Override
            public void onActivityCreated(Activity activity, Bundle bundle) {
                Log.i(TAG, "### onActivityCreated ");
            }

            @Override
            public void onActivityStarted(Activity activity) {
                Log.i(TAG, "### onActivityStarted ");
            }

            @Override
            public void onActivityResumed(Activity activity) {
                Log.i(TAG, "### onActivityResumed ");
            }

            @Override
            public void onActivityPaused(Activity activity) {
                Log.i(TAG, "### onActivityPaused2 ");
                onPause();
            }

            @Override
            public void onActivityStopped(Activity activity) {
                Log.i(TAG, "### onActivityStopped ");
            }

            @Override
            public void onActivitySaveInstanceState(Activity activity, Bundle bundle) {
                Log.i(TAG, "### onActivitySaveInstanceState ");
            }

            @Override
            public void onActivityDestroyed(Activity activity) {
                Log.i(TAG, "### onActivityDestroyed ");
            }
        });
    }

    public void onPause() {

        new AsyncTask<Void, Void, Void>() {
            protected void onPreExecute() {
                // Pre Code
            }
            protected Void doInBackground(Void... unused) {
                // Background Code

                //saving log file to firebase
                Uri file = Uri.fromFile(logFile);

                FirebaseStorage storage = FirebaseStorage.getInstance();

                // Create a storage reference from our app
                StorageReference storageRef = storage.getReference();


                StorageReference riversRef = storageRef.child("logs/" +getDeviceName() +file.getLastPathSegment());

                UploadTask uploadTask = riversRef.putFile(file);

// Register observers to listen for when the download is done or if it fails
                uploadTask.addOnFailureListener(new OnFailureListener() {
                    @Override
                    public void onFailure(@NonNull Exception exception) {
                        // Handle unsuccessful uploads
                        Log.w(TAG, "### Could not upload file ["+logFile.getName()+"] :-> " + exception.toString());
                    }
                }).addOnSuccessListener(new OnSuccessListener<UploadTask.TaskSnapshot>() {
                    @Override
                    public void onSuccess(UploadTask.TaskSnapshot taskSnapshot) {
                        // taskSnapshot.getMetadata() contains file metadata such as size, content-type, etc.
                        // ...

                        Log.w(TAG, "### Log file uploaded: " + logFile.getName() );
                    }
                });

                return null;
            }
            protected void onPostExecute(Void unused) {
                // Post Code
            }
        }.execute();





    }

    /* Checks if external storage is available for read and write */
    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if ( Environment.MEDIA_MOUNTED.equals( state ) ) {
            return true;
        }
        return false;
    }

    /* Checks if external storage is available to at least read */
    public boolean isExternalStorageReadable() {
        String state = Environment.getExternalStorageState();
        if ( Environment.MEDIA_MOUNTED.equals( state ) ||
                Environment.MEDIA_MOUNTED_READ_ONLY.equals( state ) ) {
            return true;
        }
        return false;
    }

    public static String getDeviceName() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.startsWith(manufacturer)) {
            return capitalize(model);
        }
        return capitalize(manufacturer) + " " + model;
    }

    private static String capitalize(String str) {
        if (TextUtils.isEmpty(str)) {
            return str;
        }
        char[] arr = str.toCharArray();
        boolean capitalizeNext = true;

        StringBuilder phrase = new StringBuilder();
        for (char c : arr) {
            if (capitalizeNext && Character.isLetter(c)) {
                phrase.append(Character.toUpperCase(c));
                capitalizeNext = false;
                continue;
            } else if (Character.isWhitespace(c)) {
                capitalizeNext = true;
            }
            phrase.append(c);
        }

        return phrase.toString();
    }
    private class SpectaTask extends AsyncTask<Void, Void, Void> {


        @Override
        protected Void doInBackground(Void... voids) {
            return null;
        }
    }

}
