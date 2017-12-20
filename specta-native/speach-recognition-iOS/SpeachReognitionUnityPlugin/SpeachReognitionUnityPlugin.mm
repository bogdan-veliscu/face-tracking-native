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

float *const *sampleData;
int channelCount = 1;
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
                NSLog(@"#SpechRecognition v4: Authorized :SFSpeechRecognizerAuthorizationStatusAuthorized");
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
    
    NSLog(@"## startListening");
    // Initialize the AVAudioEngine
    audioEngine = [[AVAudioEngine alloc] init];
    
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
            
            [self setRecognizedText: [NSString stringWithString: result.bestTranscription.formattedString]];
            
            isFinal = !result.isFinal;
        }
        if (error && !recognitionTask.isCancelled) {
            NSLog(@"#SpechRecognitionError: %@ %@", error, [error userInfo]);
            [audioEngine stop];
            [inputNode removeTapOnBus:0];
            recognitionRequest = nil;
            recognitionTask = nil;
        }
    }];
    
    // Sets the recording format
    AVAudioFormat *recordingFormat = [inputNode outputFormatForBus:0];
    channelCount = [recordingFormat channelCount];
    
    [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat block:^(AVAudioPCMBuffer * _Nonnull buffer, AVAudioTime * _Nonnull when) {
        [recognitionRequest appendAudioPCMBuffer:buffer];
        sampleData = buffer.floatChannelData;
    }];
    
    // Starts the audio engine, i.e. it starts listening.
    [audioEngine prepare];
    [audioEngine startAndReturnError:&error];
    NSLog(@"Say Something, I'm listening on chanel:%d", [recordingFormat channelCount]);
    
}

- (void)restartRecognition {
    sampleData = nil;
    NSLog(@"## restartRecognition");
    [self setRecognizedText: @"Initializing"];
    if (audioEngine.isRunning) {
        [self stopListening];
    }
    [self performSelector:@selector(startListening) withObject:nil afterDelay:0.3];
}

- (void) stopListening{
    NSLog(@"StopListening v2");

    // Make sure there's not a recognition task already running
    if (recognitionTask) {
        
        NSLog(@"### SpechRecognition an older recognitionTask isRunning !");
        [recognitionTask cancel];
        recognitionTask = nil;
    }
    if (audioEngine.isRunning) {
        [audioEngine stop];
        [recognitionRequest endAudio];
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
    
    void _getAudioSamples (float *samples, float *size){
        //NSLog(@"#SpechRecognition restartRecognition: %d",  channelCount);
        *size = 1024;
        if (sampleData != nil){
            for (int i = 0; i < 1024 ; i ++) {
                samples[i] = sampleData[0][i];
            }
        }
    }

    const char* _recognize(){
        //NSLog(@"#SpechRecognition _recognized: %@", [delegateObject getRecognizedText] );
        return MakeStringCopy([[delegateObject getRecognizedText] UTF8String]);
    }
    
    void _release(){
        
        NSLog(@"#SpechRecognition _release !");
        sampleData = nil;
        [delegateObject stopListening];
    }
    

}
