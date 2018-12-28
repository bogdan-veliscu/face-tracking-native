//
//  VisageTrackerUnityPlugin
//

#import "VisageTrackerUnityPlugin.h"
//
#import "OSXCameraGrabber.h"
#import <Metal/Metal.h>
#import <OpenGL/gl.h>

#include "FaceData.h"

enum VISAGE_ORIENTATION
{
    PORTRAIT = 0,
    LANDSCAPE_RIGHT = 1,
    PORTRAIT_UPSIDE_DOWN = 2,
    LANDSCAPE_LEFT = 3,
};

using namespace VisageSDK;

#define PLUGIN_TEXTURE_FORMAT GL_RGBA
#define DISPLAY_FRAME_CHANNEL_COUNT 4

static VisageTracker* m_Tracker = 0;
static OSXCameraGrabber *OSXInput = 0;

static unsigned char *pixelsRGBA = 0;
//
FaceData trackingData[MAX_FACES] = {};
FaceData FRtrackingDataBuffer[MAX_FACES] = {};
FaceData FAtrackingDataBuffer[MAX_FACES] = {};
//
int *trackerStatus = TRACK_STAT_OFF;
int FRtrackerStatusBuffer[MAX_FACES] = {};
int FAtrackerStatusBuffer[MAX_FACES] = {};
//
unsigned char* pixels = 0;
unsigned char* FRpixelsBuffer = 0;
unsigned char* FApixelsBuffer = 0;
//
int tracker_frame_format = VISAGE_FRAMEGRABBER_FMT_LUMINANCE;
//
static float xTexScale;
static float yTexScale;
// raw width and height as sent to openCamera method
int frameWidth = 0;
int frameHeight = 0;
//
int NUM_FACES = 0;
//
bool isR_initialized = false;
bool isA_initialized = false;
//
FuncPtr Debug = 0;
//
mutex mtx_fr_faceData = {};
mutex mtx_fr_pixels = {};
mutex mtx_fr_init = {};
//
mutex mtx_fa_faceData = {};
mutex mtx_fa_pixels = {};
mutex mtx_fa_init = {};

static int _isMirrored = 1;
//
const NSString *preset;

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

static int clamp(int x)
{
    unsigned y;
    return !(y=x>>8) ? x : (0xff ^ (y>>24));
}

// Returns index of the first tracker that has status TRACK_STATUS_OK, otherwise returns -1
int firstTrackerStatusOK()
{
    int index = -1;
    for (int i = 0; i<NUM_FACES; ++i)
    {
        if (trackerStatus[i] == TRACK_STAT_OK)
        {
            index = i;
            break;
        }
    }
    return index;
}

// Returns index of the first tracker that has status TRACK_STATUS_OK, TRACK_STATUS_INIT or TRACK_STAT_RECOVERING, otherwise returns -1
int firstTrackerStatusNotOFF()
{
    int index = -1;
    for (int i = 0; i<NUM_FACES; ++i)
    {
        if (trackerStatus[i] != TRACK_STAT_OFF)
        {
            index = i;
            break;
        }
    }
    return index;
}

namespace VisageSDK
{
    // prototype for licensing
    void initializeLicenseManager(const char *licenseKeyFileFolder);
}


// When native code plugin is implemented in .mm / .cpp file, then functions
// should be surrounded with extern "C" block to conform C function naming rules
extern "C" {
    
    void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height);
    void YUV_TO_RGB(unsigned char* yuv, unsigned char* buff, int width, int height);
    
    //initialize licensing
    void _initializeLicense(char* license)
    {
        initializeLicenseManager(license);
    }
    
    // initialises the tracker
    void _initTracker(char* config, int numFaces)
    {
        if (m_Tracker)
            delete m_Tracker;
        
        if (numFaces > MAX_FACES)
            NUM_FACES = MAX_FACES;
        else
            NUM_FACES = numFaces;
        
        m_Tracker = new VisageTracker(config);
    }
    
    // releases native memory allocated by tracker
    void _releaseTracker()
    {
        delete m_Tracker;
        m_Tracker = 0;
    }
    
    // attempts to initiate camera stream
    // image format is YUV420
    bool _openCamera(int orientation, int device, int width, int height, int isMirrored)
    {
        // if camera already works, release
        if (OSXInput != NULL) {
            if (OSXInput->isTracking())
                OSXInput->stopTracking();
            delete OSXInput;
            OSXInput = NULL;
            delete[] pixelsRGBA;
            pixelsRGBA = 0;
            mtx_fr_pixels.lock();
            delete[] FRpixelsBuffer;
            FRpixelsBuffer = 0;
            mtx_fr_pixels.unlock();
            mtx_fa_pixels.lock();
            delete[] FApixelsBuffer;
            FApixelsBuffer = 0;
            mtx_fa_pixels.unlock();
        }
        
        int cam_fps = 30;
        
        // initialize new camera
        if (width == -1 || height == -1)
            OSXInput = new OSXCameraGrabber(cam_fps, OSX_DEFAULT_WIDTH, OSX_DEFAULT_HEIGHT, device);
        else
            OSXInput = new OSXCameraGrabber(cam_fps, width, height, device);
        
        _isMirrored = isMirrored;
        
        long fg_pts = -1;
        
        OSXInput->startTracking();
        if (!OSXInput->isTracking())
        {
            NSLog(@"Failed to initialize camera.");
            return false;
        }
        
        OSXInput->GrabFrame(fg_pts,_isMirrored);
        
        frameWidth     = OSXInput->getWidth();
        frameHeight = OSXInput->getHeight();
        
        // prepare buffer for display image (input YUV image will be converted to RGBA)
        pixelsRGBA = new unsigned char [frameWidth*frameHeight*DISPLAY_FRAME_CHANNEL_COUNT];
        
        // prepare buffers for face analysis and face recognition (input YUV image will be converted to RGB)
        mtx_fr_pixels.lock();
        FRpixelsBuffer = new unsigned char[frameWidth * frameHeight * ANALYSIS_N_CHANNELS];
        mtx_fr_pixels.unlock();
        mtx_fa_pixels.lock();
        FApixelsBuffer = new unsigned char[frameWidth * frameHeight * ANALYSIS_N_CHANNELS];
        mtx_fa_pixels.unlock();
        
        return true;
    }
    
    // close camera stream
    bool _closeCamera()
    {
        if (OSXInput != NULL) {
            if (OSXInput->isTracking())
                OSXInput->stopTracking();
            delete OSXInput;
            OSXInput = NULL;
            delete[] pixelsRGBA;
            pixelsRGBA = 0;
            mtx_fr_pixels.lock();
            delete[] FRpixelsBuffer;
            FRpixelsBuffer = 0;
            mtx_fr_pixels.unlock();
            mtx_fa_pixels.lock();
            delete[] FApixelsBuffer;
            FApixelsBuffer = 0;
            mtx_fa_pixels.unlock();
        }
        
        return true;
    }
    
    // binds a texture with the given native hardware texture id through opengl
    void _bindTexture(int texID)
    {
        //Convert input YUV image from the OSXCameraGrabber to RGBA image to be displayed
        YUV_TO_RGBA(pixels, pixelsRGBA, frameWidth, frameHeight);
        
        glBindTexture(GL_TEXTURE_2D, texID);
        
        //Apply image to Unity texture using OpenGL
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        frameWidth,
                        frameHeight,
                        PLUGIN_TEXTURE_FORMAT,
                        GL_UNSIGNED_BYTE,
                        pixelsRGBA);
        
    }
    
    // binds a texture with the given native hardware texture id through metal
    void _bindTextureMetal(void* texID)
    {
        //Convert input YUV image from the OSXCameraGrabber to RGBA image to be displayed
        YUV_TO_RGBA(pixels, pixelsRGBA, frameWidth, frameHeight);
        
        //Apply image to Unity texture using Metal
        id<MTLTexture> mTexture;
        mTexture = (__bridge id<MTLTexture>)texID; // bridge attribute allows conversion with no refcount changes
        MTLRegion region = MTLRegionMake2D(0,0,frameWidth,frameHeight);
        [mTexture replaceRegion:region mipmapLevel:0 withBytes:pixelsRGBA bytesPerRow: frameWidth*DISPLAY_FRAME_CHANNEL_COUNT];
        
    }
    
    // grabs current frame in YUV420 format and save image data to global pixels variable
    void _grabFrame()
    {
        static long fg_pts = -1;
        pixels = OSXInput->GrabFrame(fg_pts,_isMirrored);
        frameWidth     = OSXInput->getWidth();
        frameHeight = OSXInput->getHeight();
        
        mtx_fr_init.lock();
        if (isR_initialized)
        {
            // lock possible recognition thread before copying image
            mtx_fr_pixels.lock();
            // copy image data to the face recognition buffer (YUV420-›RGB)
            YUV_TO_RGB(pixels, FRpixelsBuffer, frameWidth, frameHeight);
            mtx_fr_pixels.unlock();
        }
        mtx_fr_init.unlock();
        
        mtx_fa_init.lock();
        if (isA_initialized)
        {
            // lock possible analysis thread before copying image
            mtx_fa_pixels.lock();
            // copy image data to the face analysis buffer (YUV420-›RGB)
            YUV_TO_RGB(pixels, FApixelsBuffer, frameWidth, frameHeight);
            mtx_fa_pixels.unlock();
        }
        mtx_fa_init.unlock();
    }
    
    // estimated tracking quality
    float _getTrackingQuality(int faceIndex)
    {
        float quality = -1;
        if (trackerStatus[faceIndex] == TRACK_STAT_OK && faceIndex < NUM_FACES)
            quality = trackingData[faceIndex].trackingQuality;
        return quality;
    }
    
    // returns frame rate of the tracker
    float _getFrameRate()
    {
        float frameRate = -1;
        // find the index of first active tracker
        // NOTE: frameRate is set even if tracker status is INIT or RECOVERY
        int faceIndex = firstTrackerStatusNotOFF();
        
        if (faceIndex != -1)
            frameRate = trackingData[faceIndex].frameRate;
        
        return frameRate;
    }
    
    // Timestamp of the current video frame
    long _getTimeStamp()
    {
        long timeStamp = -1;
        
        // find the index of first active tracker
        // NOTE: timeStamp is set even if tracker status is INIT or RECOVERY
        int faceIndex = firstTrackerStatusNotOFF();
        
        if (faceIndex != -1)
            timeStamp = trackingData[faceIndex].timeStamp;
        
        return timeStamp;
    }
    
    // Scale of facial bounding box
    int _getFaceScale(int faceIndex)
    {
        if (trackerStatus[faceIndex] == TRACK_STAT_OK && faceIndex < NUM_FACES)
            return trackingData[faceIndex].faceScale;
        
        return -1;
    }
    
    // starts face tracking on current frame
    void _track()
    {
        if (!m_Tracker)
            return;
        
        // send YUV420 frame to tracker as a grayscale image
        // format is set to LUMINANCE, only first width*height pixels are relevent
        trackerStatus = m_Tracker->track(frameWidth,
                                         frameHeight,
                                         (const char *)pixels,
                                         trackingData,
                                         tracker_frame_format,
                                         VISAGE_FRAMEGRABBER_ORIGIN_TL,
                                         0,
                                         -1,
                                         NUM_FACES);
        
        if(trackerStatus[0]==TRACK_STAT_OFF && pixels)
        {
            //Set camera frame to 0 if tracker status is OFF
            memset(pixels,0,frameWidth*frameHeight);
        }
        
        // lock possible recognition thread before copying tracking results
        mtx_fr_init.lock();
        if (isR_initialized)
        {
            mtx_fr_faceData.lock();
            //
            for (int i = 0; i < NUM_FACES; ++i)
            {
                // copy face data objects to buffer
                FRtrackingDataBuffer[i] = trackingData[i];
            }
            memcpy(FRtrackerStatusBuffer, trackerStatus, NUM_FACES * sizeof(int));
            mtx_fr_faceData.unlock();
            
            mtx_fr_pixels.lock();
            if (trackerStatus[0] == TRACK_STAT_OFF && FRpixelsBuffer)
            {
                memset(FRpixelsBuffer, 0, frameWidth * frameHeight * ANALYSIS_N_CHANNELS);
            }
            mtx_fr_pixels.unlock();
        }
        mtx_fr_init.unlock();
        
        // lock possible analysis thread before copying tracking results
        mtx_fa_init.lock();
        if (isA_initialized)
        {
            mtx_fa_faceData.lock();
            for (int i = 0; i < NUM_FACES; ++i)
            {
                // copy face data objects to buffer
                FAtrackingDataBuffer[i] = trackingData[i];
            }
            memcpy(FAtrackerStatusBuffer, trackerStatus, NUM_FACES * sizeof(int));
            mtx_fa_faceData.unlock();
            
            mtx_fa_pixels.lock();
            if (trackerStatus[0] == TRACK_STAT_OFF && FApixelsBuffer)
            {
                memset(FApixelsBuffer, 0, frameWidth * frameHeight * ANALYSIS_N_CHANNELS);
            }
            mtx_fa_pixels.unlock();
        }
        mtx_fa_init.unlock();
        
        return;
    }
    
    // gets treker status
    void _getTrackerStatus(int* tStatus)
    {
        memcpy(tStatus, trackerStatus, NUM_FACES * sizeof(int));
    }
    
    // gets the current translation for the given faceIndex
    void _getHeadTranslation(float* translation, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            *(translation + 0) = -10000.0f;
            *(translation + 1) = -10000.0f;
            *(translation + 2) = 0.0f;
        }
        else
        {
            *(translation + 0) = trackingData[faceIndex].faceTranslation[0];
            *(translation + 1) = trackingData[faceIndex].faceTranslation[1];
            *(translation + 2) = trackingData[faceIndex].faceTranslation[2];
        }
    }
    
    // gets the current rotation for the given faceIndex
    void _getHeadRotation(float* rotation, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            *(rotation + 0) = 0;
            *(rotation + 1) = 0;
            *(rotation + 2) = 0;
        }
        else
        {
            *(rotation + 0) = trackingData[faceIndex].faceRotation[0];
            *(rotation + 1) = trackingData[faceIndex].faceRotation[1];
            *(rotation + 2) = trackingData[faceIndex].faceRotation[2];
        }
    }
    
    //returns number of vertices in the 3D face model.
    int _getFaceModelVertexCount()
    {
        // find the index of the first tracker with status OK
        // NOTE: faceModelVertexCount is set only if tracker status is OK
        int faceIndex = firstTrackerStatusOK();
        
        if (faceIndex == -1)
            return 0;
        
        return trackingData[faceIndex].faceModelVertexCount;
    }
    
    //returns number of triangles in the 3D face model
    int _getFaceModelTriangleCount()
    {
        // find the index of the first tracker with status OK
        // NOTE: faceModelTriangleCount is set only if tracker status is OK
        int faceIndex = firstTrackerStatusOK();
        
        if (faceIndex == -1)
            return 0;
        
        return trackingData[faceIndex].faceModelTriangleCount;
    }
    
    //returns triangles for the 3D face model.
    void _getFaceModelTriangles(int* triangles, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            return;
        }
        else
        {
            int triangleCount = trackingData[faceIndex].faceModelTriangleCount;
            for (int i = 0; i < triangleCount * 3; i++)
            {
                triangles[triangleCount * 3 - 1 - i] = trackingData[faceIndex].faceModelTriangles[i];
            }
        }
    }
    
    //returns vertex coordinates of the 3D face model
    void _getFaceModelVertices(float* vertices, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            return;
        }
        else
        {
            int vertexCount = trackingData[faceIndex].faceModelVertexCount;
            memcpy(vertices, trackingData[faceIndex].faceModelVertices, 3 * (vertexCount) * sizeof(float));
        }
    }
    
    //returns projected (image space) vertex coordinates of the 3D face model
    void _getFaceModelVerticesProjected(float* verticesProjected, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            return;
        }
        else
        {
            int vertexCount = trackingData[faceIndex].faceModelVertexCount;
            memcpy(verticesProjected, trackingData[faceIndex].faceModelVerticesProjected, 2 * (vertexCount) * sizeof(float));
        }
    }
    
    // returns texture coordinates for the 3D face model
    void _getFaceModelTextureCoords(float* texCoord, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
        {
            return;
        }
        else
        {
            xTexScale = frameWidth / (float)GetNearestPow2(frameWidth);
            yTexScale = frameHeight / (float)GetNearestPow2(frameHeight);
            int vertexCount = trackingData[faceIndex].faceModelVertexCount;
            
            for (int i = 0; i < vertexCount * 2; i += 2)
            {
                texCoord[i + 0] = (1.0f - trackingData[faceIndex].faceModelTextureCoords[i + 0]) * xTexScale;
                texCoord[i + 1] = trackingData[faceIndex].faceModelTextureCoords[i + 1] * yTexScale;
            }
        }
    }
    
    void _getTexCoordsStatic(float* buffer, int * texCoordNumber)
    {
//        for (int i = 0; i < MAX_FACES; i++)
//        {
//            if (trackerStatus[i] == TRACK_STAT_OK && trackingData[i].faceModelTextureCoordsStatic)
//            {
//                memcpy(buffer, trackingData[i].faceModelTextureCoordsStatic, trackingData[i].faceModelVertexCount * 2 * sizeof(float));
//                
//                *texCoordNumber = trackingData[i].faceModelVertexCount * 2;
//                return;
//            }
//        }
    }
    
    // returns 2
    int _getFP_START_GROUP_INDEX()
    {
        return FDP::FP_START_GROUP_INDEX;
    }
    
    // returns 15
    int _getFP_END_GROUP_INDEX()
    {
        return FDP::FP_END_GROUP_INDEX;
    }
    
    // returns size of each feature point group
    void _getGroupSizes(int* groupSize, int length)
    {
        int index = 0;
        for (int i = FDP::FP_START_GROUP_INDEX; i <= FDP::FP_END_GROUP_INDEX; ++i)
        {
            groupSize[index++] = FDP::groupSize(i);
        }
    }
    
    // returns camera info
    void _getCameraInfo(float *focus, int *width, int *height)
    {
        *focus = -1;
        *width = 0;
        *height = 0;
        
        if (!m_Tracker || OSXInput == 0)
        {
            return;
        }
        
        // find the index of first active tracker
        // NOTE: cameraFocus is set even if tracker status is INIT or RECOVERY
        int index = firstTrackerStatusNotOFF();
        
        if(index != -1)
        {
            *focus = trackingData[index].cameraFocus;
            *width = frameWidth;
            *height = frameHeight;
        }
        
        return;
    }
    
    // returns the action unit count
    int _getActionUnitCount()
    {
        // find the index of the first tracker with status OK
        // NOTE: actionUnitCount is set only if tracker status is OK
        int faceIndex = firstTrackerStatusOK();
        
        if (faceIndex != -1)
            return trackingData[faceIndex].actionUnitCount;
        
        return 0;
    }
    
    // returns all action unit values
    // if AUs au_leye_closed or au_reye_closed are disabled in the configuration file
    // it will substitute their values with discrete values of eye_closure parameter
    void _getActionUnitValues(float* values, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        // get eye au indices
        int leftIndex = -1;
        int rightIndex = -1;
        for (int i = 0; i < trackingData[faceIndex].actionUnitCount; i++)
        {
            if (!strcmp(trackingData[faceIndex].actionUnitsNames[i], "au_leye_closed"))
            {
                leftIndex = i;
            }
            
            if (!strcmp(trackingData[faceIndex].actionUnitsNames[i], "au_reye_closed"))
            {
                rightIndex = i;
            }
            
            if (leftIndex >= 0 && rightIndex >= 0)
                break;
        }
        
        // if action units for eye closure are not used by the tracker, map eye closure values to them
        if (leftIndex >= 0 && trackingData[faceIndex].actionUnitsUsed[leftIndex] == 0) {
            trackingData[faceIndex].actionUnits[leftIndex] = trackingData[faceIndex].eyeClosure[0];
        }
        if (rightIndex >= 0 && trackingData[faceIndex].actionUnitsUsed[rightIndex] == 0) {
            trackingData[faceIndex].actionUnits[rightIndex] = trackingData[faceIndex].eyeClosure[1];
        }
        
        memcpy(values, trackingData[faceIndex].actionUnits, trackingData[faceIndex].actionUnitCount * sizeof(float));
    }
    
    // returns the name of the action unit with the specified index
    const char* _getActionUnitName(int auIndex)
    {
        // find the index of the first tracker with status OK
        // NOTE: actionUnitsNames is set only if tracker status is OK
        int faceIndex = firstTrackerStatusOK();
        
        if (faceIndex != -1)
        {
            return MakeStringCopy(trackingData[faceIndex].actionUnitsNames[auIndex]);
        }
        
        return MakeStringCopy("");
    }
    
    // returns true if the action unit is used
    bool _getActionUnitUsed(int auIndex)
    {
        // find the index of the first tracker with status OK
        // NOTE: actionUnitsUsed is set only if tracker status is OK
        int faceIndex = firstTrackerStatusOK();
        
        if (faceIndex != -1)
            return trackingData[faceIndex].actionUnitsUsed[auIndex] == 1;
        
        return false;
    }
    
    // returns the gaze direction
    bool _getGazeDirection(float* direction, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return false;
        
        memcpy(direction, trackingData[faceIndex].gazeDirection, 2 * sizeof(float));
        return true;
    }
    
    // returns global gaze direction
    bool _getGazeDirectionGlobal(float* direction, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return false;
        
        memcpy(direction, trackingData[faceIndex].gazeDirectionGlobal, 3 * sizeof(float));
        return true;
    }
    
    // Returns discrete eye closure value.
    bool _getEyeClosure(float* closure, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return false;
        
        memcpy(closure, trackingData[faceIndex].eyeClosure, 2 * sizeof(float));
        return true;
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
    
    // returns feature point position in normalized 2D screen coordinates, indication of whether the point is defined and detected and quality for all feature points (6 float values for every feature point)
    // the method assumes the featurePointArray has been allocated by the callee
    void _getAllFeaturePoints2D(float* featurePointArray, int length, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float defined;
        float detected;
        
        int index = 0;
        for (int groupIndex = FDP::FP_START_GROUP_INDEX; groupIndex <= FDP::FP_END_GROUP_INDEX; ++groupIndex)
        {
            for (int pointIndex = 1; pointIndex <= FDP::groupSize(groupIndex); ++pointIndex)
            {
                if (index < length)
                {
                    const float* positions2 = trackingData[faceIndex].featurePoints2D->getFPPos(groupIndex, pointIndex);
                    featurePointArray[index++] = positions2[0];
                    featurePointArray[index++] = positions2[1];
                    
                    const FeaturePoint fp = trackingData[faceIndex].featurePoints2D->getFP(groupIndex, pointIndex);
                    
                    (fp.defined) ? defined = 1.0f : defined = 0.0f;
                    (fp.detected) ? detected = 1.0f : detected = 0.0f;
                    
                    featurePointArray[index++] = defined;
                    featurePointArray[index++] = detected;
                    featurePointArray[index++] = fp.quality;
                }
            }
        }
    }
    
    // returns the global feature point position, indication of whether the point is defined and detected and quality for all feature points (6 float values for every feature point)
    // the method assumes the featurePointArray has been allocated by the callee
    void _getAllFeaturePoints3D(float* featurePointArray, int length, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float defined;
        float detected;
        
        int index = 0;
        for (int groupIndex = FDP::FP_START_GROUP_INDEX; groupIndex <= FDP::FP_END_GROUP_INDEX; ++groupIndex)
        {
            for (int pointIndex = 1; pointIndex <= FDP::groupSize(groupIndex); ++pointIndex)
            {
                if (index < length)
                {
                    const float* positions3 = trackingData[faceIndex].featurePoints3D->getFPPos(groupIndex, pointIndex);
                    featurePointArray[index++] = positions3[0];
                    featurePointArray[index++] = positions3[1];
                    featurePointArray[index++] = positions3[2];
                    
                    const FeaturePoint fp = trackingData[faceIndex].featurePoints3D->getFP(groupIndex, pointIndex);
                    
                    (fp.defined) ? defined = 1.0f : defined = 0.0f;
                    (fp.detected) ? detected = 1.0f : detected = 0.0f;
                    
                    featurePointArray[index++] = defined;
                    featurePointArray[index++] = detected;
                    featurePointArray[index++] = fp.quality;
                }
            }
        }
    }
    
    // returns the relative 3D feature point position, indication of whether the point is defined and detected and quality for all feature points (6 float values for every feature point)
    // the method assumes the featurePointArray has been allocated by the callee
    void _getAllFeaturePoints3DRelative(float* featurePointArray, int length, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float defined;
        float detected;
        
        int index = 0;
        for (int groupIndex = FDP::FP_START_GROUP_INDEX; groupIndex <= FDP::FP_END_GROUP_INDEX; ++groupIndex)
        {
            for (int pointIndex = 1; pointIndex <= FDP::groupSize(groupIndex); ++pointIndex)
            {
                if (index < length)
                {
                    const float* positions3 = trackingData[faceIndex].featurePoints3DRelative->getFPPos(groupIndex, pointIndex);
                    featurePointArray[index++] = positions3[0];
                    featurePointArray[index++] = positions3[1];
                    featurePointArray[index++] = positions3[2];
                    
                    const FeaturePoint fp = trackingData[faceIndex].featurePoints3DRelative->getFP(groupIndex, pointIndex);
                    
                    (fp.defined) ? defined = 1.0f : defined = 0.0f;
                    (fp.detected) ? detected = 1.0f : detected = 0.0f;
                    
                    featurePointArray[index++] = defined;
                    featurePointArray[index++] = detected;
                    featurePointArray[index++] = fp.quality;
                }
            }
        }
    }
    
    // returns the feature point positions in normalized 2D screen coordinates, indication of whether the point is defined and detected and quality for specified feature points
    void _getFeaturePoints2D(int number, int* groups, int* indices, float* positions, int* defined, int* detected, float* quality, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float def;
        float det;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            
            const float* positions2 = trackingData[faceIndex].featurePoints2D->getFPPos(group, index);
            positions[i * 2] = positions2[0];
            positions[i * 2 + 1] = positions2[1];
            
            const FeaturePoint fp = trackingData[faceIndex].featurePoints2D->getFP(group, index);
            
            (fp.defined) ? def = 1.0f : def = 0.0f;
            (fp.detected) ? det = 1.0f : det = 0.0f;
            
            defined[i] = def;
            detected[i] = det;
            quality[i] = fp.quality;
        }
        
        return;
    }
    
    // returns the global 3D feature point positions, indication of whether the point is defined and detected and quality for specified feature points
    void _getFeaturePoints3D(int number, int* groups, int* indices, float* positions, int* defined, int* detected, float* quality, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float def;
        float det;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            
            const float* positions3 = trackingData[faceIndex].featurePoints3D->getFPPos(group, index);
            positions[i * 3] = positions3[0];
            positions[i * 3 + 1] = positions3[1];
            positions[i * 3 + 2] = positions3[2];
            
            const FeaturePoint fp = trackingData[faceIndex].featurePoints3D->getFP(group, index);
            
            (fp.defined) ? def = 1.0f : def = 0.0f;
            (fp.detected) ? det = 1.0f : det = 0.0f;
            
            defined[i] = def;
            detected[i] = det;
            quality[i] = fp.quality;
        }
        
        return;
    }
    
    // returns the relative 3D feature point positions, indication of whether the point is defined and detected and quality for specified feature points
    void _getFeaturePoints3DRelative(int number, int* groups, int* indices, float* positions, int* defined, int* detected, float* quality, int faceIndex)
    {
        if (trackerStatus[faceIndex] != TRACK_STAT_OK || faceIndex >= NUM_FACES)
            return;
        
        float def;
        float det;
        
        for (int i = 0; i < number; i++)
        {
            int group = groups[i];
            int index = indices[i];
            
            const float* positions3 = trackingData[faceIndex].featurePoints3DRelative->getFPPos(group, index);
            positions[i * 3] = positions3[0];
            positions[i * 3 + 1] = positions3[1];
            positions[i * 3 + 2] = positions3[2];
            
            const FeaturePoint fp = trackingData[faceIndex].featurePoints3DRelative->getFP(group, index);
            
            (fp.defined) ? def = 1.0f : def = 0.0f;
            (fp.detected) ? det = 1.0f : det = 0.0f;
            
            defined[i] = def;
            detected[i] = det;
            quality[i] = fp.quality;
        }
        
        return;
    }
    
    //sets tracking configuration
    void _setTrackerConfiguration(const char *trackerConfigFile, bool au_fitting_disabled, bool mesh_fitting_disabled)
    {
        if (m_Tracker)
            m_Tracker->setTrackerConfiguration(trackerConfigFile, au_fitting_disabled, mesh_fitting_disabled);
    }
    
    //sets the inter pupillary distance.
    void _setIPD(float IPDvalue)
    {
        if (m_Tracker)
            m_Tracker->setIPD(IPDvalue);
    }
    
    //returns the current inter pupillary distance (IPD) setting.
    float _getIPD()
    {
        float ipd = -1;
        
        if (m_Tracker)
            ipd = m_Tracker->getIPD();
        
        return ipd;
    }
    
    // helper method to convert from YUV420 to RGBA
    void YUV_TO_RGBA(unsigned char* yuv, unsigned char* buff, int width, int height)
    {
        const int frameSize = width * height;
        
        const int ii = 0;
        const int ij = 0;
        const int di = +1;
        const int dj = +1;
        
        unsigned char* rgba = buff;
        
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
                
                *rgba++ = clamp(r);
                *rgba++ = clamp(g);
                *rgba++ = clamp(b);
                *rgba++ = 255;
            }
        }
    }
    
    // helper method to convert from YUV420 to RGB
    void YUV_TO_RGB(unsigned char* yuv, unsigned char* buff, int width, int height)
    {
        const int frameSize = width * height;
        
        const int ii = 0;
        const int ij = 0;
        const int di = +1;
        const int dj = +1;
        
        unsigned char* rgb = buff;
        
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
                
                *rgb++ = clamp(r);
                *rgb++ = clamp(g);
                *rgb++ = clamp(b);
            }
        }
    }
    
}
