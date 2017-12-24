package ro.bgx.voicerecognizer;

import java.util.ArrayList;

import android.app.Activity;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.media.AudioManager;

import android.content.Context;

public class RecognizerPlugin implements
        RecognitionListener {

    private Intent recognizerIntent;
    private String LOG_TAG = "VoiceRecognitionActivity";
    private Activity context;
    private SpeechRecognizer speech = null;
    private float rmsValue = 0f;
    private String lastRecognized;
    private boolean isFinal;

    public RecognizerPlugin(Activity activity) {

        context = activity;

        context.runOnUiThread(new Runnable() {
            public void run() {

                speech = SpeechRecognizer.createSpeechRecognizer(context);
                Log.i(LOG_TAG, "_init : 1");
                speech.setRecognitionListener(RecognizerPlugin.this);

                Log.i(LOG_TAG, "_init : 2");
                recognizerIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);

                Log.i(LOG_TAG, "_init : 3");
                recognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_PREFERENCE, "en");
                Log.i(LOG_TAG, "_init : 4");

                recognizerIntent.putExtra(RecognizerIntent.EXTRA_CALLING_PACKAGE, context.getPackageName());
                Log.i(LOG_TAG, "_init : 5");

                recognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                        RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH);

                recognizerIntent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);
                Log.i(LOG_TAG, "_init : 6");
                recognizerIntent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 3);

                //recognizerIntent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_COMPLETE_SILENCE_LENGTH_MILLIS, 20000);
                //recognizerIntent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_POSSIBLY_COMPLETE_SILENCE_LENGTH_MILLIS, 20000);
            }
        });
    }

    public void _listen() {

        context.runOnUiThread(new Runnable() {
            public void run() {
                Log.i(LOG_TAG, "_listen : start");
                muteAudio();
                speech.startListening(recognizerIntent);
                Log.i(LOG_TAG, "_listen : end");
            }
        });

    }

    public void _release() {
        if (speech != null) {
            unmuteAudio();
            speech.destroy();
            Log.i(LOG_TAG, "destroy");
        }
    }

    public String _recognize(){
        return  lastRecognized;
    }

    public float _getRMS(){
        return  rmsValue;
    }

    private void muteAudio() {
        Log.i(LOG_TAG, "muteAudio");
        AudioManager amanager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        amanager.setStreamMute(AudioManager.STREAM_NOTIFICATION, true);
        amanager.setStreamMute(AudioManager.STREAM_ALARM, true);
        amanager.setStreamMute(AudioManager.STREAM_MUSIC, true);
        amanager.setStreamMute(AudioManager.STREAM_RING, true);
        amanager.setStreamMute(AudioManager.STREAM_SYSTEM, true);
    }

    private void unmuteAudio() {

        Log.i(LOG_TAG, "unmuteAudio");
        AudioManager amanager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        amanager.setStreamMute(AudioManager.STREAM_NOTIFICATION, false);
        amanager.setStreamMute(AudioManager.STREAM_ALARM, false);
        amanager.setStreamMute(AudioManager.STREAM_MUSIC, false);
        amanager.setStreamMute(AudioManager.STREAM_RING, false);
        amanager.setStreamMute(AudioManager.STREAM_SYSTEM, false);
    }

    public void onBeginningOfSpeech()
    {
        Log.i(LOG_TAG, "onBeginningOfSpeech");
    }

    @Override
    public void onBufferReceived(byte[] buffer) {

        Log.i(LOG_TAG, "onBufferReceived: " + buffer);
    }

    @Override
    public void onEndOfSpeech() {
        Log.i(LOG_TAG, "onEndOfSpeech");

        //TODO restart with a small delay??
        speech.startListening(recognizerIntent);
    }

    @Override
    public void onError(int errorCode) {
        String errorMessage = getErrorText(errorCode);
        Log.d(LOG_TAG, "FAILED " + errorMessage);
        if (errorCode == SpeechRecognizer.ERROR_SPEECH_TIMEOUT) {
            //TODO try to restart after a small dealy
            speech.startListening(recognizerIntent);
        }
    }

    @Override
    public void onEvent(int arg0, Bundle arg1) {
        Log.i(LOG_TAG, "onEvent");
    }

    @Override
    public void onPartialResults(Bundle results) {
        Log.i(LOG_TAG, "onPartialResults");
        ArrayList<String> matches = results
                .getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        lastRecognized = matches.get(0);
    }

    @Override
    public void onReadyForSpeech(Bundle arg0) {
        Log.i(LOG_TAG, "onReadyForSpeech");
        lastRecognized = "";
    }

    @Override
    public void onResults(Bundle results) {
        Log.i(LOG_TAG, "onResults");
        ArrayList<String> matches = results
                .getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        lastRecognized = matches.get(0);
    }

    @Override
    public void onRmsChanged(float rmsdB) {
        //Log.i(LOG_TAG, "onRmsChanged: " + rmsdB);
        //progressBar.setProgress((int) rmsdB);
        rmsValue = rmsdB;
    }

    public static String getErrorText(int errorCode) {
        String message;
        switch (errorCode) {
            case SpeechRecognizer.ERROR_AUDIO:
                message = "Audio recording error";
                break;
            case SpeechRecognizer.ERROR_CLIENT:
                message = "Client side error";
                break;
            case SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS:
                message = "Insufficient permissions";
                break;
            case SpeechRecognizer.ERROR_NETWORK:
                message = "Network error";
                break;
            case SpeechRecognizer.ERROR_NETWORK_TIMEOUT:
                message = "Network timeout";
                break;
            case SpeechRecognizer.ERROR_NO_MATCH:
                message = "No match";
                break;
            case SpeechRecognizer.ERROR_RECOGNIZER_BUSY:
                message = "RecognitionService busy";
                break;
            case SpeechRecognizer.ERROR_SERVER:
                message = "error from server";
                break;
            case SpeechRecognizer.ERROR_SPEECH_TIMEOUT:
                message = "No speech input";
                break;
            default:
                message = "Didn't understand, please try again.";
                break;
        }
        return message;
    }

}