namespace VisageSDK
{
    
    /*! \mainpage  visage|SDK VisageTrackerUnityPlugin project
     *
     * \htmlonly
     * <table border="0">
     * <tr>
     * <td width="32"><a href="../../../../build/msvc140/VisageTrackerUnityPlugin"><img src="../../../../../../doc/OpenGL/images/open_project_folder.png" border=0 title="Open Project Folder"></a></td>
     * <td width="32"><a href="../../../../source/VisageTrackerUnityPlugin"><img src="../../../../../../doc/OpenGL/images/open_source_folder.png" border=0 title="Open Source Code Folder"></a></td>
     * </tr>
     * </table>
     * \endhtmlonly
     * 
     * The VisageTrackerUnityPlugin is a plugin that exposes some of VisageTracker's functionality 
     * for use in managed programming languages such as C# and Java. A prebuilt version of the plugin 
     * is provided in <em>bin/VisageTrackerUnityPlugin.dll</em>. It's use is demonstrated in 
     * VisageTrackerUnity and FacialAnimationUnity samples where a C# script is used to call functions 
     * exposed in this library. This project is designed to be integrated in other applications and 
     * cannot be run on its own.
     * 
     * \section building Building the project
     * 
     * Prerequisite to building the sample project is installed Visual Studio 2015.
     * 
     * To build VisageTrackerUnityPlugin open the Visual Studio project located in Samples/OpenGL/build/msvc140/VisageTrackerUnityPlugin subfolder
     * and choose Build Solution from the Build menu.
     * 
     * \section implementation Implementation overview
     * 
     * The project consists of a Visual Studio project with C++ source code and header files.
     * It exposes some of Visage Tracker's properties such as translation, rotation and Action Units 
     * and wraps some of Visage Tracker's functions such as initiating and stopping tracking. 
     * More functionality can be exposed in the same manner. 
     * Refer to the API documentation for more information on how to access various 
     * properties and functionalities of the Visage Tracker.
     *
     * The specific classes and methods that demonstrate visage|SDK are:
     * - _initTracker(): Initializes VisageTracker object and licensing
     * - _track(): Starts face tracking on current frame
     * - _get3DData( ): Returns the current head translation, rotation and tracking status.
     * - _getFaceModel( ): Returns 3D face model data.
     * - _getFeaturePoints2D( ): Returns 2D face landmarks.
     * - _releaseTracker( ): Releases memory allocated by initTracker function.
     *
     * Some additional helper classes and methods not strictly related to the visage|SDK are:
     * - _openCamera(): Demonstrates setting up the camera stream using VideoInput library
     * - _closeCamera(): Demonstrates releasing the camera stream
     * - _setFrameData(): Demonstrates sending frame pixel data to Unity
     *
     */
}