//
//  VisageTrackerUnityPlugin.h
//  VisageTrackerUnityPlugin
//
//

#import <Foundation/Foundation.h>
#define EXPORT_API 

/** \file VisageTrackerUnityPlugin.h
 \brief Provides high-level functions for using visage|SDK face tracking capabilities in Unity game engine.
 
 It implements high-level wrapper functions around key functionalities from visage|SDK that enable usage of visage|SDK face tracking capabilities in Unity C# scripts.
 */
extern "C" { 
	/** Binds a texture with the given native hardware texture id through Metal.
	 */
	EXPORT_API void _bindTextureMetal(void* texID);
	
	/** Returns FPS.
	 */
	EXPORT_API bool _getFPS(float *fps);
	
	/** Grabs current frame.
	 */
	EXPORT_API void _grabFrame();
	
	/** Initializes new camera with the given orientation and camera information (arguments width and height currently not used).
	 */
	EXPORT_API void _openCamera(int orientation, int device, int width, int height, int isMirrored);
	
	/** Closes camera and clears memory allocated by the camera API.
	 */
	EXPORT_API void _closeCamera();
	
    /** Initialises the VisageFaceAnalyser
     int VisageSDK::VisageFaceAnalyser::init    (    const char *     dataPath    )
     
     */
    EXPORT_API void _initFaceAnalyser(char* config);
    
    /*
     
     int VisageSDK::VisageFaceAnalyser::estimateEmotion    (    VsImage *     frame,
     FaceData *     facedata,
     float *     prob_estimates
     )
     
     */
    EXPORT_API int _estimateAge();
    EXPORT_API int _estimateGender();
    
	/** Initialises the tracker.
	 */
	EXPORT_API void _initTracker(char* config, char* license);
    
    
	
	/** Releases memory allocated by the tracker in the initTracker function.
	 */
	EXPORT_API void _releaseTracker();
	
	/** Binds a texture with the given native hardware texture id through OpenGL.
	 */
	EXPORT_API void _bindTexture(int texID);
	
	/** Returns the current head translation, rotation and tracking status.
	 */
	EXPORT_API const char* _get3DData(float* tx, float* ty, float* tz,float* rx, float* ry, float* rz);
	
	/** Performs face tracking on current frame and returns tracker status.
	 */
    EXPORT_API int _track();
    
    
    EXPORT_API void _refreshAgeEstimate();
    
	
	/** Returns camera info.
	 */
	EXPORT_API void _getCameraInfo(float *focus, int *width, int *height);
	
	/** Returns data needed to draw 3D face model.
	 */
	EXPORT_API bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord);
	
	/** Returns the action unit count.
	 */
	EXPORT_API int _getActionUnitCount();
	
	/** Returns the name of the action unit with the specified index.
	 */
	EXPORT_API const char* _getActionUnitName(int index);
	
	/** Returns true if the action unit is used.
	 */
	EXPORT_API bool _getActionUnitUsed(int index);
	
	/** Returns all action unit values.
	 */
	EXPORT_API void _getActionUnitValues(float* values);
	
	/** Returns the gaze direction.
	 */
	EXPORT_API bool _getGazeDirection(float* direction);

	/**Returns the feature point positions in normalized 2D screen coordinates.
	*/
	EXPORT_API bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions);
	
	/** Returns the global 3d feature point positions in meters.
	*/
	EXPORT_API bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions);
	
	/** Returns the relative 3d feature point positions in meters.
	*/
	EXPORT_API bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions);
	
    
    /// -------------- QR scanner new methods

    typedef void (*callbackFunc)(const char *);
    typedef void (*transitionCallback)();
    
    
    EXPORT_API void _initScanner(transitionCallback initCallback, callbackFunc scanCallback);
    
    /** Releases memory allocated by the scanner in the initScanner function.
     */
    EXPORT_API void _releaseScanner(transitionCallback callback);
    
    EXPORT_API void _toggleTorch(int on);

}

