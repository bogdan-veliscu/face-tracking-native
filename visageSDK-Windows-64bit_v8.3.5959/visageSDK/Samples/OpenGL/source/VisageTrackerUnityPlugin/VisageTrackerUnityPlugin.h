//
//  VisageTrackerUnityPlugin.h
//  VisageTrackerUnityPlugin
//
//

#ifdef WIN32
#ifdef _MSC_VER
// ensures valid dll function export
#define EXPORT_API __declspec(dllexport)
#endif
#endif

/** \file VisageTrackerUnityPlugin.h
 \brief Provides high-level functions for using visage|SDK face tracking capabilities in Unity game engine.
 
 It implements high-level wrapper functions around key functionalities from visage|SDK that enable usage of visage|SDK face tracking capabilities in Unity C# scripts.
 */
extern "C" {
	
	/** Updates the pixel data in Unity with data from tracker.
	 */
	EXPORT_API void _setFrameData(char* imageData);
	
	/** Grabs current frame.
	*/
	EXPORT_API void _grabFrame();

	/** Initializes camera with the given orientation and camera information.
	*/
	EXPORT_API void _openCamera(int orientation, int device, int width, int height, int isMirrored);

	/** Closes camera and clears memory allocated by the camera API.
	*/
	EXPORT_API void _closeCamera();
	
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
	EXPORT_API const char* _get3DData(float* tx, float* ty, float* tz, float* rx, float* ry, float* rz);

	/** Performs face tracking on current frame and returns tracker status.
	*/
	EXPORT_API int _track();
	
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

	/** Returns the feature point positions in normalized 2D screen coordinates.
	*/
	EXPORT_API bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions);

	/** Returns the global 3d feature point positions in meters.
	*/
	EXPORT_API bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions);

	/** Returns the relative 3d feature point positions in meters.
	*/
	EXPORT_API bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions);

}
