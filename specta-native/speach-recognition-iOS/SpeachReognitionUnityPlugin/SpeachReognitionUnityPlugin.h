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
    // Keeps track of search status
    NSString* lastRecognizedText;
    
    SFSpeechRecognizer *speechRecognizer;
    SFSpeechAudioBufferRecognitionRequest *recognitionRequest;
    SFSpeechRecognitionTask *recognitionTask;
    AVAudioEngine *audioEngine;

}

@property (nonatomic, copy) NSString* lastRecognizedText;

@end

extern "C" {
    /** Binds a texture with the given native hardware texture id through Metal.
     */
    EXPORT_API void _init();
    
    EXPORT_API void _listen();
    
    EXPORT_API const char* _recognize();
    
    EXPORT_API void _release();    
    
}
