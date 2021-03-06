//
//  VisageTrackerUnityPlugin.mm
//

#import "VisageTrackerUnityPlugin.h"
//#import "VisageTracker.h"
//
#import "Utilities.h"
#import "CameraGrabber.h"
#import "UIDeviceHardware.h"
#import <Metal/Metal.h>
#import <OpenGLES/ES1/gl.h>

#define PLUGIN_TEXTURE_FORMAT GL_RGBA
#define CHANNEL_COUNT 4

using namespace VisageSDK;

char* trackerConfig;
char* trackerLicense;
char* analyserConfig;
char* analyserLicense;

static callbackFunc scanCallback;

static transitionCallback trackerInitCallback;
static transitionCallback analyserInitCallback;

static VisageTracker* m_Tracker = 0;
static CameraGrabber *cameraGrabber = 0;

static VsImage *m_Frame = 0;
static VisageFaceAnalyser *m_FaceAnalizer = 0;
pthread_mutex_t mFrameMutex;
static int detectedAge = -2;
static int detectedGender = -2;
static int ageRefreshRequested = 1;

static unsigned char *pixelsRGBA = 0;
//
static FaceData trackingData[4];
static int *trackerStatus = TRACK_STAT_OFF;
//
static float xTexScale;
static float yTexScale;
static int frameWidth;
static int frameHeight;
static int cam_width;
static int cam_height;
//
static unsigned char* pixels = 0;
static unsigned char* pixelsFA = 0;
int FAframeWidth = 0;
int FAframeHeight = 0;

int format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
static int _rotated = 0;
static int _isMirrored = 1;
//
const NSString *preset;
//
static const float m_pi = 3.14159265f;
static const float inv_pi = 1.0f/m_pi;

static BOOL scannerEnabled = false;
static BOOL switchingCamera = false;

static BOOL frameReady = false;

static int framesToFade = 0;
static int maxFramesToFade = 15;

static float * detectedEmotions = new float [7];

static int clamp(int x)
{
    unsigned y;
    return !(y=x>>8) ? x : (0xff ^ (y>>24));
}

// Helper method to create C string copy
static char* MakeStringCopy (const char* val)
{
    if (val == NULL)
        return NULL;
    
    char* res = (char*)malloc(strlen(val) + 1);
    strcpy(res, val);
    return res;
}

static unsigned int GetNearestPow2(unsigned int num)
{
    unsigned int n = num > 0 ? num - 1 : 0;
    
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    
    return n;
}

namespace VisageSDK
{
    // prototype for licensing
    void initializeLicenseManager(const char *licenseKeyFileFolder);
}

enum Orientation
{
    VISAGE_PORTRAIT,
    VISAGE_LANDSCAPE,
    VISAGE_PORTRAIT_FLIPPED,
    VISAGE_LANDSCAPE_FLIPPED
};

// When native code plugin is implemented in .mm / .cpp file, then functions
// should be surrounded with extern "C" block to conform C function naming rules
extern "C" {
    
    void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height);
    
    void _initTracker (char* config, char* license)
    {
        _initTrackerWithCallback(config, license, nullptr);
        NSLog(@"### _initTracker v3");
    }
    
    void _initTrackerWithCallback(char* config, char* license, transitionCallback callback){
        NSLog(@"### _initTrackerWithCallback v5");
        trackerConfig = MakeStringCopy(config);
        trackerLicense = MakeStringCopy(license);
        trackerInitCallback = callback;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            
            NSLog(@"### _initTrackerWithCallback ----- 1");
            initializeLicenseManager(trackerLicense);
            
            NSLog(@"### _initTrackerWithCallback ----- 2");
            if (m_Tracker)
                _releaseTracker();
            
            NSLog(@"### _initTrackerWithCallback ----- config: %s", trackerConfig);
            m_Tracker = new VisageTracker(trackerConfig);
            NSLog(@"### _initTrackerWithCallback ----- 4");
            if (trackerInitCallback != NULL){
                NSLog(@"### _initTracker callback");
                trackerInitCallback();
            }
        });
    }
    void _releaseTracker()
    {
        delete m_Tracker;
        delete m_FaceAnalizer;
        vsReleaseImage(&m_Frame);
        m_Tracker = 0;
    }
    
    void _initFaceAnalyser(char* config, char* license){
        _initFaceAnalyserWithCallback(config, license, nullptr);
    }
    
    EXPORT_API void _initFaceAnalyserWithCallback(char* config, char* license, transitionCallback callbackFunc){
        
        analyserConfig = MakeStringCopy(config);
        analyserLicense = MakeStringCopy(license);
        analyserInitCallback = callbackFunc;
        pthread_mutex_init(&mFrameMutex, NULL);
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            NSLog(@"### _initFaceAnalyserWithCallback v3 ");
            
            initializeLicenseManager(analyserLicense);
            
            if (m_FaceAnalizer){
                delete m_FaceAnalizer;
            }
            m_FaceAnalizer = new VisageFaceAnalyser();
            NSLog(@"@@ VisageFaceAnalyser initi with config: [%s]\n", "Data/Raw/Visage Tracker/bdtsdata/LBF/vfadata/");
            
            NSBundle *mainBundle = [NSBundle mainBundle];
            NSString *myFile = [mainBundle pathForResource:@"Data/Raw/Visage Tracker/bdtsdata/LBF/vfadata" ofType: @""];
            
            
            analyserConfig = MakeStringCopy([myFile UTF8String]);
            NSLog(@"@@ 2222 VisageFaceAnalyser initi with config: [%s]\n", analyserConfig);
            
            int ret = m_FaceAnalizer->init(analyserConfig);
            NSLog(@"### VisageFaceAnalyser _initFaceAnalyser :%d", ret);
            if (analyserInitCallback != NULL){
                
                
                NSLog(@"### VisageFaceAnalyser _initFaceAnalyser CALLBACK :%d", ret);
                analyserInitCallback();
            }
        });
    }
    
    void _refreshAgeEstimate(){
        ageRefreshRequested = 1;
    }
    
    int _estimateAge(){
        
        return detectedAge;
        
    }
    int _estimateGender(){
        return detectedGender;
    }
    
    void _estimateEmotion(float *emotions){
        
        for (int i=0; i<7; i++) {
            emotions[i] = detectedEmotions[i];
        }
    }
    
    void _openCamera(int orientation, int device, int width2, int height, int isMirrored)
    {
        NSLog(@"#### _openCamera --- [%d] <%d>", device, orientation);
        NSString* deviceType = [UIDeviceHardware platform];
        
        int cam_fps = 30;
        
        if (orientation == VISAGE_PORTRAIT || orientation == VISAGE_PORTRAIT_FLIPPED)
        {
            // portrait mode
            cam_width = 720;
            cam_height = 1280;
            preset = AVCaptureSessionPreset1280x720;
        }
        else
        {
            // landscape mode
            cam_width = 1280;
            cam_height = 720;
            preset = AVCaptureSessionPreset1280x720 ;
        }
        
        // override for iPhone 4
        if ([deviceType hasPrefix:@"iPhone3"]) {	// iPhone4
            if (orientation == VISAGE_PORTRAIT || orientation == VISAGE_PORTRAIT_FLIPPED)
            {
                // portrait mode
                cam_width = 144;
                cam_height = 192;
                preset = AVCaptureSessionPresetLow;
            }
            else
            {
                // landscape mode
                cam_width = 192;
                cam_height = 144;
                preset = AVCaptureSessionPresetLow;
            }
            
        }
        
        // if camera already works, release
        if (cameraGrabber)
        {
            [cameraGrabber stopCapture];
            cameraGrabber = 0;
            delete[] pixelsRGBA;
            pixelsRGBA = 0;
        }
        
        _rotated = orientation;
        _isMirrored = isMirrored;
        
        pixelsRGBA = new unsigned char [cam_width*cam_height*CHANNEL_COUNT];
        
        
        
        // initialize new camera
        cameraGrabber = [[CameraGrabber alloc] initWithSessionPreset:preset UseFPS:cam_fps withDevice:device];
        [cameraGrabber startCapture: nil];
        
        int pixelFormat = [cameraGrabber getPixelFormat];
        
        if (pixelFormat == 1)
            format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
        else
            format = VISAGE_FRAMEGRABBER_FMT_BGRA;
        
        //NSLog(@"#### _openCamera ---  111 %d x %d --> %p <> %p", frameWidth, frameHeight, pixelsFA , pixels);
        pthread_mutex_lock(&mFrameMutex);
        frameWidth     = cam_width;
        frameHeight = cam_height;
        
        if (FAframeWidth != frameWidth || FAframeHeight != frameHeight){
            delete[] pixelsFA;
            pixelsFA = new unsigned char[frameWidth * frameHeight * CHANNEL_COUNT];
            FAframeWidth = frameWidth;
            FAframeHeight = frameHeight;
            NSLog(@"#### _openCamera - pixels intialized %d x %d", frameWidth, frameHeight);
            
            if (m_Frame)
            {
                vsReleaseImageHeader(&m_Frame);
                //NSLog(@"#### vsReleaseImageHeader(&m_Frame);");
            }
            m_Frame = vsCreateImageHeader(vsSize(FAframeWidth, FAframeHeight), 8, CHANNEL_COUNT);
            
            //NSLog(@"#### vsCreateImageHeader(vsSize(FAframeWidth, FAframeHeight), 8, CHANNEL_COUNT);");
        }
        
        //NSLog(@"#### _openCamera ---  222 %d x %d --> %p <> %p [%c]", frameWidth, frameHeight, pixelsFA , pixels, pixelsFA[0]);
        pthread_mutex_unlock(&mFrameMutex);
        //frameReady = true;
    }
    
    // closes camera
    void _closeCamera()
    {
        if(cameraGrabber)
            
        {   [cameraGrabber stopCapture];
            cameraGrabber = 0;
            delete[] pixelsRGBA;
            pixelsRGBA = 0;
        }
        
        pthread_mutex_lock(&mFrameMutex);
        
        if(m_Frame){
            //NSLog(@"#### BEFORE delete %d x %d --> %p <> %p", frameWidth, frameHeight, pixelsFA , pixels);
            NSLog(@"#### realeasing frame 22");
            //vsReleaseImage(&m_Frame);
            //m_Frame = 0;
            if(pixelsFA){
                delete[] pixelsFA;
                pixelsFA = 0;
            }
            
            frameReady = false;
            framesToFade = maxFramesToFade;
            ageRefreshRequested = false;
            //NSLog(@"#### AFTER delete %d x %d --> %p <> %p", frameWidth, frameHeight, pixelsFA , pixels);
        }
        pthread_mutex_unlock(&mFrameMutex);
    }
    
    // binds a texture with the given native hardware texture id through opengl
    void _bindTexture(int texID)
    {
        if (format == VISAGE_FRAMEGRABBER_FMT_LUMINANCE) {
            //Convert to RGBA for image to be drawn
            YUV_TO_RGBA(pixels, pixelsRGBA, frameWidth, frameHeight);
        }
        else {
            memcpy(pixelsRGBA, pixels, frameWidth*frameHeight*CHANNEL_COUNT);
        }
        
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameWidth, frameHeight, PLUGIN_TEXTURE_FORMAT, GL_UNSIGNED_BYTE, pixelsRGBA);
    }
    
    // binds a texture with the given native hardware texture id through metal
    void _bindTextureMetal(void* texID)
    {
        if (pixelsRGBA == 0 || pixels == 0){
            
            return;
        }
        if(switchingCamera){
            memset(pixelsRGBA, 0, frameWidth * frameHeight);
            framesToFade = maxFramesToFade;
        } else {
            
            if (format == VISAGE_FRAMEGRABBER_FMT_LUMINANCE) {
                //Convert to RGBA for image to be drawn
                YUV_TO_RGBA(pixels, pixelsRGBA, frameWidth, frameHeight);
            }
            else {
                memcpy(pixelsRGBA, pixels, frameWidth*frameHeight*CHANNEL_COUNT);
            }
        }
        id<MTLTexture> mTexture;
        mTexture = (__bridge id<MTLTexture>)texID; // bridge attribute allows conversion with no refcount changes
        MTLRegion region = MTLRegionMake2D(0,0,frameWidth,frameHeight);
        [mTexture replaceRegion:region mipmapLevel:0 withBytes:pixelsRGBA bytesPerRow: frameWidth*CHANNEL_COUNT];
    }
    
    // grabs current frame
    void _grabFrame()
    {
        
        if(switchingCamera){
            return;
        }
        
        pthread_mutex_lock(&mFrameMutex);
        pixels = [cameraGrabber getBuffer:_rotated isMirrored:_isMirrored];
        
        if(m_Frame){
        frameReady = true;
        frameWidth     = cam_width;
        frameHeight = cam_height;
        }
        pthread_mutex_unlock(&mFrameMutex);
    }
    
    // starts face tracking on current frame
    int _track()
    {
        
        if(!scannerEnabled){
            trackerStatus = m_Tracker->track(frameWidth, frameHeight, (const char *)pixels, trackingData,format);
            
            pthread_mutex_lock(&mFrameMutex);
            if(ageRefreshRequested && trackerStatus[0] == TRACK_STAT_OK && pixelsRGBA && m_Frame != 0 && frameReady) {
                ageRefreshRequested =0;
                
                if (!pixelsFA){
                   pixelsFA = new unsigned char[frameWidth * frameHeight * CHANNEL_COUNT];
                }
                if (FAframeWidth != frameWidth || FAframeHeight != frameHeight){
                    delete[] pixelsFA;
                    pixelsFA = new unsigned char[frameWidth * frameHeight * CHANNEL_COUNT];
                    FAframeWidth = frameWidth;
                    FAframeHeight = frameHeight;
                    //NSLog(@"#### FAframeWidth != frameWidth || FAframeHeight != frameHeight %d x %d", frameWidth, frameHeight);
                    
                    if (m_Frame)
                    {
                        vsReleaseImageHeader(&m_Frame);
                        //NSLog(@"#### vsReleaseImageHeader(&m_Frame);");
                    }
                    m_Frame = vsCreateImageHeader(vsSize(FAframeWidth, FAframeHeight), 8, CHANNEL_COUNT);
                    
                    //NSLog(@"#### vsCreateImageHeader(vsSize(FAframeWidth, FAframeHeight), 8, CHANNEL_COUNT);");
                }
                //NSLog(@"#### BEFORE memcpy %d x %d --> %p <> %p", frameWidth, frameHeight, pixelsFA , pixels);
                memcpy(pixelsFA, pixelsRGBA, frameWidth*frameHeight*CHANNEL_COUNT);
                
                //NSLog(@"#### memcpy(pixelsFA, pixels, frameWidth*frameHeight*CHANNEL_COUNT); %p, %p", pixelsFA, pixels);

                vsSetImageData(m_Frame, (void*)pixelsFA, m_Frame->widthStep);
                
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    pthread_mutex_lock(&mFrameMutex);
                    if (m_FaceAnalizer && m_Frame != 0 && frameReady) {
                        detectedAge = m_FaceAnalizer->estimateAge(m_Frame, &trackingData[0]);
                        detectedGender = m_FaceAnalizer->estimateGender(m_Frame, &trackingData[0]);
                        
                        m_FaceAnalizer->estimateEmotion(m_Frame, &trackingData[0], detectedEmotions);
                        //NSLog(@"#### Detected age:%d and gender: %d", detectedAge, detectedGender);
                    }
                    pthread_mutex_unlock(&mFrameMutex);
                });
            }
            
            pthread_mutex_unlock(&mFrameMutex);
        } else {
            trackerStatus[0] = TRACK_STAT_RECOVERING;
        }
        
        if(trackerStatus[0] == TRACK_STAT_OFF && pixels)
        {
            if (format == VISAGE_FRAMEGRABBER_FMT_LUMINANCE)
                memset(pixels,0,frameWidth*frameHeight);
            else
                memset(pixels,0,frameWidth*frameHeight*CHANNEL_COUNT);
        }
        
        return trackerStatus[0];
    }
    
    // gets the current translation
    const char* _get3DData(float* tx, float* ty, float* tz,float* rx, float* ry, float* rz)
    {
        if (trackerStatus[0] != TRACK_STAT_OK) {
            *tx = -10000.0f;
            *ty = -10000.0f;
            *tz = 0.0f;
        } else {
            *tx = -trackingData[0].faceTranslation[0];
            *ty = trackingData[0].faceTranslation[1];
            *tz = trackingData[0].faceTranslation[2];
        }
        *rx = trackingData[0].faceRotation[0]*180.0f*inv_pi;
        *ry = -(trackingData[0].faceRotation[1]+m_pi)*180.0f*inv_pi;
        *rz = -trackingData[0].faceRotation[2]*180.0f*inv_pi;
        
        const char *tstatus;
        
        switch (trackerStatus[0]) {
            case TRACK_STAT_OFF:
                tstatus = "OFF";
                break;
            case TRACK_STAT_OK:
                tstatus = "OK";
                break;
            case TRACK_STAT_RECOVERING:
                tstatus = "RECOVERING";
                break;
            case TRACK_STAT_INIT:
                tstatus = "INITIALIZING";
                break;
            default:
                tstatus = "N/A";
                break;
        }
        
        char message[256];
        sprintf(message, " %4.1f FPS  Status: %s\n Head position %+5.1f %+5.1f %+5.1f \n Rotation (deg) %+5.1f %+5.1f %+5.1f \n\n",
                trackingData[0].frameRate,
                tstatus,
                trackingData[0].faceTranslation[0],
                trackingData[0].faceTranslation[1],
                trackingData[0].faceTranslation[2],
                trackingData[0].faceRotation[0],
                trackingData[0].faceRotation[1],
                trackingData[0].faceRotation[2]);
        
        return MakeStringCopy(message);
    }
    
    void _getCameraInfo(float *focus, int *width, int *height)
    {
        *focus = trackingData[0].cameraFocus;
        *width = frameWidth;
        *height = frameHeight;
    }
    
    // a helper function to get all the needed info in one native call
    bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord)
    {
        if (trackerStatus == TRACK_STAT_OFF)
            return false;
        
        // get vertex number
        *vertexNumber = trackingData[0].faceModelVertexCount;
        
        // get vertices
        memcpy(vertices, trackingData[0].faceModelVertices, 3*(*vertexNumber)*sizeof(float));
        
        // get triangle number
        *triangleNumber = trackingData[0].faceModelTriangleCount;
        
        // get triangles in reverse order
        for(int i = 0; i < *triangleNumber * 3; i++)
        {
            triangles[*triangleNumber * 3 - 1 - i] = trackingData[0].faceModelTriangles[i];
        }
        
        //texture coordinates are normalized to frame
        //and because frame is only in the part of our texture
        //we must scale texture coordinates to match
        //frame_width/tex_width and frame_height/tex_height
        //also x-coord must be flipped
        
        
        xTexScale = frameWidth / (float) GetNearestPow2(frameWidth);
        yTexScale = frameHeight / (float) GetNearestPow2(frameHeight);
        
        
        for (int i = 0; i < *vertexNumber*2; i+=2) {
            texCoord[i+0] = (1.0f - trackingData[0].faceModelTextureCoords[i+0]) * xTexScale;
            texCoord[i+1] = trackingData[0].faceModelTextureCoords[i+1] * yTexScale;
        }
        
        return true;
    }
    
    int _getActionUnitCount()
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return 0;
        
        return trackingData[0].actionUnitCount;
    }
    
    void _getActionUnitValues(float* values)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return;
        
        // get eye au indices
        int leftIndex = -1;
        int rightIndex = -1;
        for (int i = 0; i < trackingData[0].actionUnitCount; i++)
        {
            if (!strcmp(trackingData[0].actionUnitsNames[i], "au_leye_closed"))
            {
                leftIndex = i;
            }
            
            if (!strcmp(trackingData[0].actionUnitsNames[i], "au_reye_closed"))
            {
                rightIndex = i;
            }
            
            if (leftIndex >= 0 && rightIndex >= 0)
                break;
        }
        
        // if action units for eye closure are not used by the tracker, map eye closure values to them
        if (leftIndex >= 0 && trackingData[0].actionUnitsUsed[leftIndex] == 0) {
            trackingData[0].actionUnits[leftIndex] = trackingData[0].eyeClosure[0];
        }
        if (rightIndex >= 0 && trackingData[0].actionUnitsUsed[rightIndex] == 0) {
            trackingData[0].actionUnits[rightIndex] = trackingData[0].eyeClosure[1];
        }
        
        memcpy(values, trackingData[0].actionUnits, trackingData[0].actionUnitCount * sizeof(float));
    }
    
    const char* _getActionUnitName(int index)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return MakeStringCopy("");
        
        return MakeStringCopy(trackingData[0].actionUnitsNames[index]);
    }
    
    bool _getActionUnitUsed(int index)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        
        return trackingData[0].actionUnitsUsed[index] == 1;
    }
    
    bool _getGazeDirection(float* direction)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        
        memcpy(direction, trackingData[0].gazeDirection, 2 * sizeof(float));
        return true;
    }
    
    bool _getFPS(float *fps)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        *fps = trackingData[0].frameRate;
        return true;
    }
    
    bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            const float* position = trackingData[0].featurePoints2D->getFPPos(group, index);
            positions[i*2] = position[0];
            positions[i*2 + 1] = position[1];
        }
        
        return true;
    }
    
    
    // returns the global feature point position, indication of whether the point is defined and detected and quality for all feature points (6 float values for every feature point)
    // the method assumes the featurePointArray has been allocated by the callee
    bool _getAllFeaturePoints3D(ActionUnitStruct* featurePointArray, int length)
    {
        
        //NSLog(@"### _getAllFeaturePoints3D");
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        NSLog(@"### _getAllFeaturePoints3D tracking OK");
        int index = 0;
        for (int groupIndex = FDP::FP_START_GROUP_INDEX; groupIndex <= FDP::FP_END_GROUP_INDEX; ++groupIndex)
        {
            for (int pointIndex = 1; pointIndex <= FDP::groupSize(groupIndex); ++pointIndex)
            {
                if (index < length)
                {
                    
                    const float* positions3 = trackingData->featurePoints3D->getFPPos(groupIndex, pointIndex);
                    featurePointArray[index].posX = positions3[0];
                    featurePointArray[index].posY = positions3[1];
                    featurePointArray[index].posZ = positions3[2];
                    
                    const FeaturePoint fp = trackingData->featurePoints3D->getFP(groupIndex, pointIndex);
                    
                    featurePointArray[index].defined = fp.defined;
                    featurePointArray[index].detected = fp.detected;
                    featurePointArray[index].quality = fp.quality;
                    
                    featurePointArray[index].index = pointIndex;
                    featurePointArray[index].group = groupIndex;
                    index ++;
                } else {
                    //NSLog(@"### _getAllFeaturePoints3D --- OVERFLOW ?");
                    break;
                }
            }
        }
        
        
        NSLog(@"### _getAllFeaturePoints3D tracking  ---- COMPLETED");
        return true;
    }
    
    
    bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            const float* position = trackingData[0].featurePoints3D->getFPPos(group, index);
            positions[i * 3] = position[0];
            positions[i * 3 + 1] = position[1];
            positions[i * 3 + 2] = position[2];
        }
        
        return true;
    }
    
    bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions)
    {
        if (trackerStatus[0] != TRACK_STAT_OK)
            return false;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            const float* position = trackingData[0].featurePoints3DRelative->getFPPos(group, index);
            positions[i * 3] = position[0];
            positions[i * 3 + 1] = position[1];
            positions[i * 3 + 2] = position[2];
        }
        
        return true;
    }
    
    void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height)
    {
        const int frameSize = width * height;
        
        const int ii = 0;
        const int ij = 0;
        const int di = +1;
        const int dj = +1;
        
        unsigned char* rgba = buff;
        float fadeFactor = 1;
        if(framesToFade > 0) {
            fadeFactor = (float)(maxFramesToFade-framesToFade)/maxFramesToFade;
            
            framesToFade--;
        }
        for (int i = 0, ci = ii; i < height; ++i, ci += di)
        {
            for (int j = 0, cj = ij; j < width; ++j, cj += dj)
            {
                int y = (0xff & ((int) yuv[ci * width + cj]));
                int v = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 0]));
                int u = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 1]));
                y = y < 16 ? 16 : y;
                
                int a0 = 1192 * (y -  16);
                int a1 = 1634 * (v - 128);
                int a2 =  832 * (v - 128);
                int a3 =  400 * (u - 128);
                int a4 = 2066 * (u - 128);
                
                int r = (a0 + a1) >> 10;
                int g = (a0 - a2 - a3) >> 10;
                int b = (a0 + a4) >> 10;
                r = (int) (fadeFactor *r);
                g = (int) (fadeFactor *g);
                b = (int) (fadeFactor *b);
                
                *rgba++ = clamp(r);
                *rgba++ = clamp(g);
                *rgba++ = clamp(b);
                *rgba++ = 255;
                
            }
        }
    }
    
    static void SetCallback() {
        [cameraGrabber setCompletionWithBlock:^(NSString *resultAsString) {
            CGRect *rect = cameraGrabber.qrFrame;
            
            NSLog(@"###  SCAN onScan: %@ , at frame: %@", resultAsString, NSStringFromCGRect(*rect));
            if (scanCallback != NULL){
                Rect *myRect = new Rect{(short)rect->origin.x,(short)rect->origin.y,
                    (short)rect->size.width,(short)rect->size.height};
                NSLog(@"### QR rect bounds: %d, %d | %d, %d", myRect->top, myRect->left,
                      myRect->bottom, myRect->right );
                scanCallback([resultAsString UTF8String],
                             rect->origin.y, rect->origin.x, rect->size.height, rect->size.width );
            }
        }];
    }
    
    void _initScanner(transitionCallback initCallback, callbackFunc callback){
        NSLog(@"### QR SCAN _initScanner - ASYNC --- v4.2");
        scanCallback = callback;
        scannerEnabled = true;
        switchingCamera = true;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            
            _openCamera(VISAGE_PORTRAIT, 1,cam_width, cam_height, true);
            
            NSLog(@"### QR SCAN _initScanner - ASYNC - returned");
            if (initCallback != NULL){
                initCallback();
            }
            if (cameraGrabber){
                NSLog(@"### QR SCAN _initScanner - actual init start");
                [cameraGrabber initScanner];
                switchingCamera = false;
                SetCallback();
                [cameraGrabber startScanning];
                NSLog(@"### VisageFaceAnalyser _initScanner : completed!");
            }
        });
    }
    
    /** Releases memory allocated by the scanner in the initScanner function.
     */
    void _releaseScanner(transitionCallback callback){
        switchingCamera = true;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            if (cameraGrabber){
                
                NSLog(@"### QR SCAN _releaseScanner - ASYNC - before the actual release");
                [cameraGrabber stopScanning];
                
                _openCamera(VISAGE_PORTRAIT, 0,cam_width, cam_height, true);
                switchingCamera = false;
                NSLog(@"### QR SCAN _releaseScanner - ASYNC - before callback");
                if (callback != NULL){
                    callback();
                }
                scannerEnabled = false;
            }
        });
    }
    
    void _toggleTorch(int on){
        [cameraGrabber toggleTorch];
    }
    
    void _tapToFocus (float x, float y){
        
        [cameraGrabber focusOnPoint: CGPointMake(x, y)];
    }
}
