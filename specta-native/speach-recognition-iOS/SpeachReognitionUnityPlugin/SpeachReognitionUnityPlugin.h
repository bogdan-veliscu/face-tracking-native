//
//  SpeachReognitionUnityPlugin.h
//  SpeachReognitionUnityPlugin
//
//  Created by Bogdan Veliscu on 10/25/17.
//  Copyright © 2017 Bogdan Veliscu. All rights reserved.
//


#import <Foundation/Foundation.h>
#define EXPORT_API

@interface SpeachReognitionDelegate : NSObject<SFSpeechRecognizerDelegate> {

    SFSpeechRecognizer *speechRecognizer;
    SFSpeechAudioBufferRecognitionRequest *recognitionRequest;
    SFSpeechRecognitionTask *recognitionTask;
    AVAudioEngine *audioEngine;
}

@end

extern "C" {
    /** Binds a texture with the given native hardware texture id through Metal.
     */
    
    typedef void (*callbackFunc)(const char *);
    
    EXPORT_API void _init();
    
    EXPORT_API void _listen(callbackFunc scanCallback);
    
    EXPORT_API void _release();
    
    EXPORT_API void _getAudioSamples (float *samples, float *size);
    
}
