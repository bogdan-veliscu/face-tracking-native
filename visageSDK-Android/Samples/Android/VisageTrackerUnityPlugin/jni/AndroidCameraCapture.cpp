#include "AndroidCameraCapture.h"
#include <android/log.h>
#include <sys/time.h>


namespace VisageSDK
{

AndroidCameraCapture::AndroidCameraCapture()
{
	pts = 0;
}

AndroidCameraCapture::AndroidCameraCapture(int width, int height, int orientation, int flip)
{
	buffer = vsCreateImage(vsSize(width, height),VS_DEPTH_8U,3);
	bufferN = vsCreateImage(vsSize(width, height),VS_DEPTH_8U,3);
	bufferT = vsCreateImage(vsSize(height, width),VS_DEPTH_8U,3);
	pthread_mutex_init(&guardFrame_mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pts = 0;
	frameArrived = false;
	this->orientation = orientation;
	this->flip = flip;
	this->width = width;
	this->height = height;	
}

AndroidCameraCapture::~AndroidCameraCapture(void)
{
	// cleaning up
	vsReleaseImage(&buffer);
	vsReleaseImage(&bufferT);
	vsReleaseImage(&bufferN);
	pthread_mutex_destroy(&guardFrame_mutex);
	pthread_cond_destroy(&cond);	
}

void AndroidCameraCapture::WriteFrameYUV(unsigned char* imageData)
{
	pthread_mutex_lock(&guardFrame_mutex);
	YUV_NV21_TO_RGB(imageData, buffer, width, height);
	frameArrived = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&guardFrame_mutex);	
}

unsigned char *AndroidCameraCapture::GrabFrame(long &timeStamp)
{
    struct timespec   ts;
    struct timeval    tp;
    unsigned char* ret;

    int rc = gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec + 2;
    ts.tv_nsec = 0;
	pthread_mutex_lock(&guardFrame_mutex);
	while (!frameArrived){
		int ret_cond = pthread_cond_timedwait(&cond, &guardFrame_mutex, &ts);
        if (ret_cond == ETIMEDOUT){
            pthread_mutex_unlock(&guardFrame_mutex);
            return 0;
    	}
    }
    frameArrived = false;
	
	switch(orientation){
		case 0: case 360:
			if (flip)
				vsFlip(buffer, bufferN, 1);	
			else
				vsCopy(buffer, bufferN);
			ret = (unsigned char*) bufferN->imageData;	
			break;
		case 90:
			vsTranspose(buffer, bufferT);
			if (!flip)
				vsFlip(bufferT, bufferT, 1);
			ret = (unsigned char*) bufferT->imageData;	
			break;
		case 180:
			if (flip)
				vsFlip(buffer, bufferN, 0);
			else
				vsFlip(buffer, bufferN, -1);
			ret = (unsigned char*) bufferN->imageData;
			break;
		case 270:
			vsTranspose(buffer, bufferT);
			if (flip)
				vsFlip(bufferT, bufferT, -1);
			else
				vsFlip(bufferT, bufferT, 0);
			ret = (unsigned char*) bufferT->imageData;	
			break;
	}

	pthread_mutex_unlock(&guardFrame_mutex);
	timeStamp = pts++;
	return ret;
}

int AndroidCameraCapture::clamp(int x) {
    unsigned y;
    return !(y=x>>8) ? x : (0xff ^ (y>>24));
}

void AndroidCameraCapture::YUV_NV21_TO_RGB(unsigned char* yuv, VsImage* buff, int width, int height)
{
    const int frameSize = width * height;

    const int ii = 0;
    const int ij = 0;
    const int di = +1;
    const int dj = +1;

    unsigned char* rgb = (unsigned char*)buff->imageData;

    int a = 0;
    for (int i = 0, ci = ii; i < height; ++i, ci += di)
    {
        for (int j = 0, cj = ij; j < width; ++j, cj += dj)
        {
            int y = (0xff & ((int) yuv[ci * width + cj]));
            int v = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 0]));
            int u = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 1]));
            y = y < 16 ? 16 : y;

            //int r = (int) (1.164f * (y - 16) + 1.596f * (v - 128));
            //int g = (int) (1.164f * (y - 16) - 0.813f * (v - 128) - 0.391f * (u - 128));
            //int b = (int) (1.164f * (y - 16) + 2.018f * (u - 128));

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
