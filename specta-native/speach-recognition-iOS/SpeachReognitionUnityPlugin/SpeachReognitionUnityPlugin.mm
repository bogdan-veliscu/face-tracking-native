//
//  SpeachReognitionUnityPlugin.m
//  SpeachReognitionUnityPlugin
//
//  Created by Bogdan Veliscu on 10/25/17.
//  Copyright © 2017 Bogdan Veliscu. All rights reserved.
//


#import <Speech/Speech.h>
#import "SpeachReognitionUnityPlugin.h"
@implementation SpeachReognitionDelegate
//@synthesize lastRecognizedText;

- (id)init
{
    self = [super init];
    lastRecognizedText = @"Initializing";
    
    // Initialize the Speech Recognizer with the locale, couldn't find a list of locales
    // but I assume it's standard UTF-8 https://wiki.archlinux.org/index.php/locale
    speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"]];
    
    // Set speech recognizer delegate
    speechRecognizer.delegate = self;
    
    // Request the authorization to make sure the user is asked for permission so you can
    // get an authorized response, also remember to change the .plist file, check the repo's
    // readme file or this projects info.plist
    [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
        switch (status) {
            case SFSpeechRecognizerAuthorizationStatusAuthorized:
                NSLog(@"#SpechRecognition: Authorized :SFSpeechRecognizerAuthorizationStatusAuthorized");
                break;
            case SFSpeechRecognizerAuthorizationStatusDenied:
                NSLog(@"#SpechRecognition: Denied");
                break;
            case SFSpeechRecognizerAuthorizationStatusNotDetermined:
                NSLog(@"#SpechRecognition: Not Determined");
                break;
            case SFSpeechRecognizerAuthorizationStatusRestricted:
                NSLog(@"#SpechRecognition: Restricted");
                break;
            default:
                break;
        }
    }];
    
    return self;
}

- (void)startListening {
    
    // Initialize the AVAudioEngine
    audioEngine = [[AVAudioEngine alloc] init];
    
    // Make sure there's not a recognition task already running
    if (recognitionTask) {
        
        NSLog(@"#SpechRecognition an older recognitionTask isRunning !");
        [recognitionTask cancel];
        recognitionTask = nil;
    }
    
    // Starts an AVAudio Session
    NSError *error;
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryRecord error:&error];
    [audioSession setActive:YES withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&error];
    
    // Starts a recognition process, in the block it logs the input or stops the audio
    // process if there's an error.
    recognitionRequest = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
    AVAudioInputNode *inputNode = audioEngine.inputNode;
    recognitionRequest.shouldReportPartialResults = YES;
    recognitionTask = [speechRecognizer recognitionTaskWithRequest:recognitionRequest resultHandler:^(SFSpeechRecognitionResult * _Nullable result, NSError * _Nullable error) {
        BOOL isFinal = NO;
        if (result) {
            // Whatever you say in the mic after pressing the button should be being logged
            // in the console.
            NSLog(@"#SpechRecognition RESULT: %@",result.bestTranscription.formattedString);
            
            [self setRecognizedText: [NSString stringWithString: result.bestTranscription.formattedString]];
            
            //([result.bestTranscription.formattedString UTF8String]);
            isFinal = !result.isFinal;
        }
        if (error) {
            NSLog(@"#SpechRecognitionError: %@ %@", error, [error userInfo]);
            [audioEngine stop];
            [inputNode removeTapOnBus:0];
            recognitionRequest = nil;
            recognitionTask = nil;
        }
    }];
    
    // Sets the recording format
    AVAudioFormat *recordingFormat = [inputNode outputFormatForBus:0];
    [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat block:^(AVAudioPCMBuffer * _Nonnull buffer, AVAudioTime * _Nonnull when) {
        [recognitionRequest appendAudioPCMBuffer:buffer];
    }];
    
    // Starts the audio engine, i.e. it starts listening.
    [audioEngine prepare];
    [audioEngine startAndReturnError:&error];
    NSLog(@"Say Something, I'm listening");
    
}

- (void)restartRecognition {
    
    [self setRecognizedText: @"Initializing"];
    if (audioEngine.isRunning) {
        [recognitionRequest endAudio];
        [audioEngine stop];
    }
    [self performSelector:@selector(startListening) withObject:nil afterDelay:0.3];
}

- (void) stopListening{
    NSLog(@"StopListening");
    if (audioEngine.isRunning) {
        [recognitionRequest endAudio];
        [audioEngine stop];
    }
    // Make sure there's not a recognition task already running
    if (recognitionTask) {
        
        NSLog(@"#SpechRecognition an older recognitionTask isRunning !");
        [recognitionTask cancel];
        recognitionTask = nil;
    }
}

- (NSString *)getRecognizedText
{
    return lastRecognizedText;
}

- (void)setRecognizedText : (NSString*) newText;
{
    //NSLog(@"### setRecognizedText : %@", newText);
    lastRecognizedText = newText;
}

#pragma mark - SFSpeechRecognizerDelegate Delegate Methods

- (void)speechRecognizer:(SFSpeechRecognizer *)speechRecognizer availabilityDidChange:(BOOL)available {
    NSLog(@"#SpechRecognition: Availability:%d",available);
}

@end

static SpeachReognitionDelegate* delegateObject = nil;

// Helper method to create C string copy
char* MakeStringCopy (const char* string)
{
    if (string == NULL)
        return NULL;
    
    char* res = (char*)malloc(strlen(string) + 1);
    strcpy(res, string);
    return res;
}

extern "C" {
    
    void _init(){
        if (delegateObject == nil){
            delegateObject = [[SpeachReognitionDelegate alloc] init];
        }
    }
    
    void _listen(){
        NSLog(@"#SpechRecognition restartRecognition");
        [delegateObject restartRecognition];
    }
    

    const char* _recognize(){
        //NSLog(@"#SpechRecognition _recognized: %@", [delegateObject getRecognizedText] );
        return MakeStringCopy([[delegateObject getRecognizedText] UTF8String]);
    }
    
    void _release(){
        
        NSLog(@"#SpechRecognition _release !");
        
        [delegateObject stopListening];
    }
    

}
