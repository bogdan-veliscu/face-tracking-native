//
//	VisageTrackerUnityPlugin
//

namespace VisageSDK
{
	void __declspec(dllimport) initializeLicenseManager(const char *licenseKeyFileFolder);
}

#include "VisageTrackerUnityPlugin.h"
#include "VisageTracker.h"
//
#include "videoInput.h"

using namespace VisageSDK;

static VisageTracker* m_Tracker = 0;
static videoInput *VI = 0;
static FaceData trackingData;
//
static int *trackerStatus = TRACK_STAT_OFF;
//
unsigned char* pixels = 0;
unsigned char* pixelsRaw = 0;
int format = VISAGE_FRAMEGRABBER_FMT_RGB;
static int frameWidth;
static int frameHeight;
//
static int cam_device = 0;
static int cameraMirrored = 1;
static int cameraOrientation = 0;
static float xTexScale;
static float yTexScale;

// Helper method to create C string copy
static char* MakeStringCopy(const char* val)
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

// Helper method to rotate/mirror RGB image
static void rotateRGB(unsigned char* input, unsigned char* output, int width, int height, int rotation, int cameraMirrored)
{
	bool swap = (rotation == 1);
	bool xflip = false;
	bool yflip = false;

	if (cameraMirrored == 1)
		xflip = true;

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			int rIn = j * width * 3 + 3 * i + 0;
			int gIn = j * width * 3 + 3 * i + 1;
			int bIn = j * width * 3 + 3 * i + 2;

			int wOut = swap ? height : width;
			int hOut = swap ? width : height;
			int iSwapped = swap ? j : i;
			int jSwapped = swap ? i : j;
			int iOut = xflip ? wOut - iSwapped - 1 : iSwapped;
			int jOut = yflip ? hOut - jSwapped - 1 : jSwapped;

			int rOut = jOut * wOut * 3 + 3 * iOut + 0;
			int gOut = jOut * wOut * 3 + 3 * iOut + 1;
			int bOut = jOut * wOut * 3 + 3 * iOut + 2;

			output[rOut] = input[rIn];
			output[gOut] = input[gIn];
			output[bOut] = input[bIn];
		}
	}
}

// When native code plugin is implemented in .mm / .cpp file, then functions
// should be surrounded with extern "C" block to conform C function naming rules
extern "C" {

	// initialises the tracker
	void _initTracker(char* config, char* license)
	{
		//initialize licensing
		//example how to initialize license key
		initializeLicenseManager(license);

		if (m_Tracker)
			delete m_Tracker;

		m_Tracker = new VisageTracker(config);
	}

	// grabs current frame
	void _grabFrame()
	{
		if (!VI) return;

		while (!VI->isFrameNew(cam_device))
			Sleep(1);

		VI->getPixels(cam_device, (unsigned char*)pixelsRaw, false, true);

		rotateRGB(pixelsRaw, pixels, frameWidth, frameHeight, cameraOrientation, cameraMirrored);

	}

	// release memory allocated by the initTracker function
	void _releaseTracker()
	{
		delete m_Tracker;
		m_Tracker = 0;
	}

	// starts face tracking on current frame
	int _track()
	{
		trackerStatus = m_Tracker->track(frameWidth, frameHeight, (const char *)pixels, &trackingData, format);

		if (trackerStatus[0] == TRACK_STAT_OFF && pixels)
			memset(pixels, 0, frameWidth*frameHeight * 3);

		return trackerStatus[0];
	}

	//  initializes new camera
	void _openCamera(int orientation, int device, int width, int height, int isMirrored)
	{
		int cam_width = 800;
		int cam_height = 600;
		int cam_fps = 30;
		int format = VISAGE_FRAMEGRABBER_FMT_RGB;
		bool r = false;

		int n = videoInput::listDevices();

		if (n > device)
			cam_device = device;
		else
			cam_device = 0;

		// if camera already works, release
		if (VI != 0)
		{
			VI->stopDevice(cam_device);
			delete VI; VI = 0; 
			delete[] pixels;
			delete[] pixelsRaw;
		}
		// ***** Grabbing from camera using Videoinput library *******
		VI = new videoInput();

		VI->setIdealFramerate(cam_device, cam_fps);
		
		if (width == -1 || height == -1)
			 r = VI->setupDevice(cam_device, cam_width, cam_height);
		else
			 r = VI->setupDevice(cam_device, width, height);

		if (!r)
		{
			VI->stopDevice(cam_device);
			delete VI; VI = 0;
			MessageBox(0,"Error opening camera! Check if camera is already running.","ERROR",MB_ICONERROR);
			return;
		}

		cam_width = VI->getWidth(cam_device);
		cam_height = VI->getHeight(cam_device);

		printf("Start tracking from camera %dx%d@%d\n", cam_width, cam_height, cam_fps);

		frameWidth = cam_width;
		frameHeight = cam_height;

		pixels = new unsigned char[frameWidth*frameHeight * 3];
		VI->getPixels(cam_device, (unsigned char*)pixels, false, true);


		pixelsRaw = new unsigned char[frameWidth*frameHeight * 3];
		cameraMirrored = isMirrored;
		cameraOrientation = orientation;
	}

	// closes camera
	void _closeCamera()
	{
		if (VI != 0)
		{
			VI->stopDevice(cam_device);
			delete VI; VI = 0;
			delete[] pixels;
			delete[] pixelsRaw;
			pixels = 0;
			pixelsRaw = 0;
		}
	}

	// binds a texture with the given native hardware texture id through opengl
	void _bindTexture(int texID)
	{
		//glBindTexture(GL_TEXTURE_2D, texID);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameWidth, frameHeight, PLUGIN_TEXTURE_FORMAT, GL_UNSIGNED_BYTE, pixels);
	}

	// gets the current translation
	const char* _get3DData(float* tx, float* ty, float* tz, float* rx, float* ry, float* rz)
	{

		if (trackerStatus[0] != TRACK_STAT_OK) {
			*tx = -10000.0f;
			*ty = -10000.0f;
			*tz = 0.0f;
		}
		else {
			*tx = -trackingData.faceTranslation[0];
			*ty = trackingData.faceTranslation[1];
			*tz = trackingData.faceTranslation[2];
		}
		*rx = trackingData.faceRotation[0]*180/3.14f;
		*ry = -(trackingData.faceRotation[1]+3.14f)*180.0f/3.14f;
		*rz = -trackingData.faceRotation[2]*180/3.14f;

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
			trackingData.frameRate,
			tstatus,
			trackingData.faceTranslation[0],
			trackingData.faceTranslation[1],
			trackingData.faceTranslation[2],
			trackingData.faceRotation[0],
			trackingData.faceRotation[1],
			trackingData.faceRotation[2]);

		return MakeStringCopy(message);
	}

	// returns camera info
	void _getCameraInfo(float *focus, int *width, int *height)
	{
		*focus = trackingData.cameraFocus;
		*width = frameWidth;
		*height = frameHeight;
	}

	// a helper function to get all the needed info in one native call
	bool _getFaceModel(int* vertexNumber, float* vertices, int* triangleNumber, int* triangles, float* texCoord)
	{
		if (trackerStatus == TRACK_STAT_OFF)
			return false;

		// get vertex number
		*vertexNumber = trackingData.faceModelVertexCount;

		// get vertices
		memcpy(vertices, trackingData.faceModelVertices, 3 * (*vertexNumber)*sizeof(float));

		// get triangle number
		*triangleNumber = trackingData.faceModelTriangleCount;

		// get triangles in reverse order
		for (int i = 0; i < *triangleNumber * 3; i++)
		{
			triangles[*triangleNumber * 3 - 1 - i] = trackingData.faceModelTriangles[i];
		}

		//texture coordinates are normalized to frame
		//and because frame is only in the part of our texture
		//we must scale texture coordinates to match
		//frame_width/tex_width and frame_height/tex_height
		//also x-coord must be flipped


		xTexScale = frameWidth / (float)GetNearestPow2(frameWidth);
		yTexScale = frameHeight / (float)GetNearestPow2(frameHeight);


		for (int i = 0; i < *vertexNumber * 2; i += 2) {
			texCoord[i + 0] = (1.0f - trackingData.faceModelTextureCoords[i + 0]) * xTexScale;
			texCoord[i + 1] = trackingData.faceModelTextureCoords[i + 1] * yTexScale;
		}

		return true;
	}

	// returns the action unit count
	int _getActionUnitCount()
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return 0;

		return trackingData.actionUnitCount;
	}

	// returns all action unit values
	void _getActionUnitValues(float* values)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return;

		// get eye au indices
		int leftIndex = -1;
		int rightIndex = -1;
		for (int i = 0; i < trackingData.actionUnitCount; i++)
		{
			if (!strcmp(trackingData.actionUnitsNames[i], "au_leye_closed"))
			{
				leftIndex = i;
			}

			if (!strcmp(trackingData.actionUnitsNames[i], "au_reye_closed"))
			{
				rightIndex = i;
			}

			if (leftIndex >= 0 && rightIndex >= 0)
				break;
		}

		// if action units for eye closure are not used by the tracker, map eye closure values to them
		if (leftIndex >= 0 && trackingData.actionUnitsUsed[leftIndex] == 0) {
			trackingData.actionUnits[leftIndex] = trackingData.eyeClosure[0];
		}
		if (rightIndex >= 0 && trackingData.actionUnitsUsed[rightIndex] == 0) {
			trackingData.actionUnits[rightIndex] = trackingData.eyeClosure[1];
		}

		memcpy(values, trackingData.actionUnits, trackingData.actionUnitCount * sizeof(float));
	}

	// returns the name of the action unit with the specified index
	const char* _getActionUnitName(int index)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return MakeStringCopy("");

		return MakeStringCopy(trackingData.actionUnitsNames[index]);
	}

	// returns true if the action unit is used
	bool _getActionUnitUsed(int index)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return false;

		return trackingData.actionUnitsUsed[index] == 1;
	}

	// returns the gaze direction
	bool _getGazeDirection(float* direction)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return false;

		memcpy(direction, trackingData.gazeDirection, 2 * sizeof(float));
		return true;
	}

	// returns the feature point positions in normalized 2D screen coordinates
	bool _getFeaturePoints2D(int number, int* groups, int* indices, float* positions)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData.featurePoints2D->getFPPos(group, index);
			positions[i * 2] = position[0];
			positions[i * 2 + 1] = position[1];
		}

		return true;
	}

	// returns the global 3d feature point positions
	bool _getFeaturePoints3D(int number, int* groups, int* indices, float* positions)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData.featurePoints3D->getFPPos(group, index);
			positions[i * 3] = position[0];
			positions[i * 3 + 1] = position[1];
			positions[i * 3 + 2] = position[2];
		}

		return true;
	}

	// returns the relative 3d feature point positions
	bool _getFeaturePoints3DRel(int number, int* groups, int* indices, float* positions)
	{
		if (trackerStatus[0] != TRACK_STAT_OK)
			return false;

		for (int i = 0; i < number; i++)
		{
			int group = groups[i];
			int index = indices[i];
			const float* position = trackingData.featurePoints3DRelative->getFPPos(group, index);
			positions[i * 3] = position[0];
			positions[i * 3 + 1] = position[1];
			positions[i * 3 + 2] = position[2];
		}

		return true;
	}

	// updates the pixel data in Unity with data from tracker
	void _setFrameData(char* imageData)
	{
		int texWidth = GetNearestPow2(frameWidth);
		int texHeight = GetNearestPow2(frameHeight);
		int dim = frameWidth*frameHeight;
		for (int i = 0; i < frameHeight; i++) {
			for (int j = 0; j < frameWidth; j++) {
				imageData[i * 4 * texWidth + j * 4 + 0] = pixels[i * 3 * frameWidth + j * 3 + 0];
				imageData[i * 4 * texWidth + j * 4 + 1] = pixels[i * 3 * frameWidth + j * 3 + 1];
				imageData[i * 4 * texWidth + j * 4 + 2] = pixels[i * 3 * frameWidth + j * 3 + 2];
				imageData[i * 4 * texWidth + j * 4 + 3] = -1;
			}
		}
	}
}
