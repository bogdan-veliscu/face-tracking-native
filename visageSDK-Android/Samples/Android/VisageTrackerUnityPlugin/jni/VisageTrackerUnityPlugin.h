//
//  VisageTrackerUnityPlugin.h
//  VisageTrackerUnityPlugin
//
//

/** \file VisageTrackerUnityPlugin.h
 \brief Provides high-level functions for using visage|SDK face tracking capabilities in Unity game engine.
 
 It implements high-level wrapper functions around key functionalities from visage|SDK that enable usage of visage|SDK face tracking capabilities in Unity C# scripts.
 */



extern "C" {
	
	void AlertCallback(const char* warningMessage);

	/** Passes pointer to frame from camera
	*/
	void _writeFrame(void * pixelData);
	
	/** Initialises the tracker.
	 */
	void _initTracker(char* configuration, char* license);

    void _initFaceAnalyser(char* config, char* license);

    void _refreshAgeEstimate();
    int _estimateAge();
    int _estimateGender();
	
	/** Releases memory allocated by the tracker in the initTracker function.
	*/
	void _releaseTracker();
	
	/** Binds a texture with the given native hardware texture id through OpenGL.
	 */
	void _bindTexture(int texID);

	/** Returns the current head translation, rotation and tracking status.
	 */
	const char* _get3DData(float* tx, float* ty, float* tz,float* rx, float* ry, float* rz);
	
	/** Performs face tracking on current frame and returns tracker status.
	*/
	int _track();

	/** Returns camera info.
	 */
	void _getCameraInfo(float *focus, int *ImageWidth, int *ImageHeight);
	
	/** Returns data needed to draw 3D face model.
	 */
	bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord);

	/** Returns the action unit count.
	 */
	int _getActionUnitCount();
	
	/** Returns the name of the action unit with the specified index.
	 */
	const char* _getActionUnitName(int index);
	
	/** Returns true if the action unit is used.
	 */
	bool _getActionUnitUsed(int index);
	
	/** Returns all action unit values.
	 */
	void _getActionUnitValues(float* values);
	
	/** Returns the gaze direction.
	 */
	bool _getGazeDirection(float* direction);

	/** Returns the feature point positions in normalized 2D screen coordinates.
	*/
	bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions);	
	
	/** Returns the global 3d feature point positions in meters.
	*/
	bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions);
	
	/** Returns the relative 3d feature point positions in meters.
	*/
	bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions);

    void _getRawFrame(int* camHeight, int *camWidth, char *rawData);

    void initializeOpenGL();

    void resizeViewport(int newWidth, int newHeight);

    void renderFrame();

    int getTexturePointer();
}