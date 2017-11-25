#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <vector>
#include <stdio.h>
#include "VisageDrawing.h"
#include "VisageFeaturesDetector.h"
#include "Common.h"
#include "WrapperOpenCV.h"
#include "cv.h"
#include "VisageFaceRecognition.h"

#include <android/log.h>
#define  LOG_TAG    "DetectorWrapper"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define MAX_SIZE 768


/** \file DrawingWrapper.cpp
 * Uses OpenGL to display image processed by VisageFaceRecognition or VisageFeaturesDetector functionality.
 *
 * Key members of wrapper are:
 * - visageFaceRecogntion: the VisageFaceRecogntion object
 * - displayIdentity: method that demonstrates how to display data obtained from VisageFaceRecognition
 *
 */

extern "C" {

bool _isDisplayed = false;
GLuint texIds[3];

/**
* Texture ID for displaying frames.
*/
GLuint frameTexId = 0;
GLuint logoTexId = 0;

/**
* Image to be drawn.
*/
IplImage *_drawImage = 0;
IplImage *_logoImage = 0;

/**
* Texture coordinates for displaying frames.
*/
float xTexCoord;

/**
* Texture coordinates for displaying frames.
*/
float yTexCoord;

/**
* Size of the texture for displaying frames.
*/
int xTexSize;

/**
* Size of the texture for displaying frames.
*/
int yTexSize;

/**
* Aspect of the video.
*/
float videoAspect;

/**
* Size of the OpenGL view.
*/
int glWidth;

/**
* Size of the OpenGL view.
*/
int glHeight;

/**
 * Method for initializing video texture through OpenGL.
 *
 * The texture must have dimensions that are power of 2, so video data will cover only part of the texture.
 *
 * @param image frame to initialize video display with
 */
void initFrameTexture(IplImage *image)
{
	xTexSize = 1;
	yTexSize = 1;
	while((image->width / xTexSize) > 0) xTexSize <<= 1;
	while((image->height / yTexSize) > 0) yTexSize <<= 1;

	glGenTextures(1, &frameTexId);


	glBindTexture(GL_TEXTURE_2D, frameTexId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	switch (image->nChannels)
	{
		case 1:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, xTexSize, yTexSize, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
			break;
		case 3:
		case 4:
		default:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xTexSize, yTexSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			break;
	}
	xTexCoord = image->width / (float) xTexSize;
	yTexCoord = image->height / (float) yTexSize;

	videoAspect = image->width / (float) image->height;
	float tmp;
	if(image->width < image->height)
	{
		tmp = glHeight;
		glHeight = glWidth / videoAspect;
		if (glHeight > tmp)
		{
			glWidth  = glWidth*tmp/glHeight;
			glHeight = tmp;
		}
	}
	else
	{
		tmp = glWidth;
		glWidth = glHeight * videoAspect;
		if (glWidth > tmp)
		{
			glHeight  = glHeight*tmp/glWidth;
			glWidth = tmp;
		}
	}
}

/**
 * Method for refreshing video texture through OpenGL.
 *
 * The part of the texture containing video data is refreshed with current frame data.
 *
 * @param image frame to refresh video display with
 */
void setFrameTexture(IplImage *image)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, frameTexId);
	switch (image->nChannels)
	{
		case 3:
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_RGB, GL_UNSIGNED_BYTE, image->imageData);
			break;
		case 4:
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_BGRA, GL_UNSIGNED_BYTE, image->imageData);
			break;
		case 1:
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_LUMINANCE, GL_UNSIGNED_BYTE, image->imageData);
			break;
		default:
			return;
		}
}

/**
 * Method for displaying video through OpenGL.
 */
void DrawImage()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();									// Reset The Projection Matrix

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();									// Reset The Projection Matrix
	glOrthof(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

	const GLfloat vertices[] = {
		-1.0f,-1.0f,-5.0f,
		1.0f,-1.0f,-5.0f,
		-1.0f,1.0f,-5.0f,
		1.0f,1.0f,-5.0f,
	};

	//tex coords are flipped upside down instead of an image
	const GLfloat texcoords[] = {
		0.0f,		yTexCoord,
		xTexCoord,	yTexCoord,
		0.0f,		0.0f,
		xTexCoord,	0.0f,
	};

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);					// Set Color To White

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	glViewport(0, 0, glWidth, glHeight);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glClear(GL_DEPTH_BUFFER_BIT);
}

void DisplayLogo(const VsImage *logo, int width, int height)
{
	//Create the texture if not inited
	if(logoTexId == 0)
	{
		glGenTextures(1, &logoTexId);

		//Bind the newly created texture
		glBindTexture(GL_TEXTURE_2D, logoTexId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//Creating texture
#ifdef IOS
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logo->width, logo->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, logo->imageData);
#elif ANDROID
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logo->width, logo->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, logo->imageData);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logo->width, logo->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, logo->imageData);
#endif
	}

	 glBindTexture(GL_TEXTURE_2D, logoTexId);

#if defined(WIN32) || defined(LINUX)
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_STENCIL_BUFFER_BIT | GL_TRANSFORM_BIT | GL_TEXTURE_BIT );
#endif

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_FOG);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	//transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	//use texture
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

#if defined(WIN32) || defined(LINUX) || defined(MAC_OS_X)
	glOrtho(0.0f,1.0f,0.0f,1.0f,-10.0f,10.0f);
#else
	glOrthof(0.0f,1.0f,0.0f,1.0f,-10.0f,10.0f);
#endif

	//logo aspect
	float logoAspect = logo->width / (float)logo->height;
	//viewport aspect
	float viewportAspect = width / (float)height;
	//set logo position to upper right corner, maintain logo aspect relative
	float x = 0.75f;
	float y = 1 - ((1 - x) * viewportAspect / logoAspect);

	GLfloat vertices[] = {
		x,y,-5.0f,
		1.0f,y,-5.0f,
		x,1.0f,-5.0f,
		1.0f,1.0f,-5.0f,
	};

	//tex coords are flipped upside down instead of an image
	GLfloat texcoords[] = {
		0.0f,1.0f,
		1.0f,1.0f,
		0.0f,0.0f,
		1.0f,0.0f,
	};

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	glViewport(0, 0, width, height);

	//drawing vertices and texcoords
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	//disable logo texture
	glBindTexture(GL_TEXTURE_2D, 0);

#if defined(WIN32) || defined(LINUX)
	glPopAttrib();
#endif

	//glClear(GL_DEPTH_BUFFER_BIT);
}

/**
 * Method for displaying image used for detection/recognition through OpenGL ES..
 *
 * This method is called by the application rendering thread to display detection/recognition results.
 * @param width width of GLSurfaceView used for rendering.
 * @param height height of GLSurfaceView used for rendering.
 */
void Java_com_visagetechnologies_facedetectdemo_DetectorRenderer_drawFrame(JNIEnv* env, jobject obj, jint width, jint height)
{
	 if(!_isDisplayed)
	 {
		 glWidth = width;
		 glHeight = height;
		 _isDisplayed = true;
	 }

	if(frameTexId == 0)
	{
		initFrameTexture(_drawImage);
	}

	setFrameTexture(_drawImage);

	DrawImage();
	DisplayLogo((const VsImage*)_logoImage, glWidth, glHeight);
}



}