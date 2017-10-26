#include "VisageDrawing.h"


namespace VisageSDK
{

void VisageDrawing::drawResults(VsImage* img, FaceData* td, bool drawOnly2DFeatures)
{

		static int contourPoints[] = {
			13,	1,
			13,	3,
			13,	5,
			13,	7,
			13,	9,
			13,	11,
			13,	13,
			13,	15,
			13,	17,
			13,	16,
			13,	14,
			13,	12,
			13,	10,
			13,	8,
			13,	6,
			13,	4,
			13,	2,
		};

		drawSpline2D(img, contourPoints, td->featurePoints2D, 17);
		drawPoints2D(img, contourPoints, td->featurePoints2D, 17, 0, td);

		static int outerUpperLipPoints[] = {
			8, 4,
			8, 6,
			8, 9,
			8, 1,
			8, 10,
			8, 5,
			8, 3,
		};
		drawSpline2D(img, outerUpperLipPoints, td->featurePoints2D, 7);

		static int outerLowerLipPoints[] = {
			8, 4,
			8, 8,
			8, 2,
			8, 7,
			8, 3,
		};
		drawSpline2D(img, outerLowerLipPoints, td->featurePoints2D, 5);

		static int innerUpperLipPoints[] = {
			2, 5,
			2, 7,
			2, 2,
			2, 6,
			2, 4,
		};
		drawSpline2D(img, innerUpperLipPoints, td->featurePoints2D, 5);

		static int innerLowerLipPoints[] = {
			2, 5,
			2, 9,
			2, 3,
			2, 8,
			2, 4,
		};
		drawSpline2D(img, innerLowerLipPoints, td->featurePoints2D, 5);

		static int innerLipPoints[] = {
			2,	2,
			2,	6,
			2,	4,
			2,	8,
			2,	3,
			2,	9,
			2,	5,
			2,	7,
		};

		drawPoints2D(img, innerLipPoints, td->featurePoints2D, 8, 0, td);

		static int outerLipPoints[] = {
			8,	1,
			8,	10,
			8,	5,
			8,	3,
			8,	7,
			8,	2,
			8,	8,
			8,	4,
			8,	6,
			8,	9,
		};

		drawPoints2D(img, outerLipPoints, td->featurePoints2D, 10, 0, td);

		static int noseLinePoints[] = {
			9,	1,
			9,	3,
			9,	2
		};
		drawSpline2D(img, noseLinePoints, td->featurePoints2D, 3);

		static int nosePoints[] = {
			9,	1,
			9,	2,
			9,	3,
			9,	15
		};

		drawPoints2D(img, nosePoints, td->featurePoints2D, 4, 0, td);

		//draw eyes
		if (td->eyeClosure[0] > 0.5f){
			static int irisPoints1[] = {
				3,	5
			};
			drawPoints2D(img, irisPoints1, td->featurePoints2D, 1, 0, td);
		}
		if (td->eyeClosure[1] > 0.5f){
			static int irisPoints[] = {
				3,	6
			};
			drawPoints2D(img, irisPoints, td->featurePoints2D, 1, 0, td);
		}

		static int outerUpperEyePointsR[] = {
			3,	12,
			3,	14,
			3,	8
		};

		static int outerLowerEyePointsR[] = {
			3,	8,
			3,	10,
			3,	12
		};

		static int innerUpperEyePointsR[] = {
			3,	12,
			12, 10,
			3,	2,
			12, 6,
			3,	8
		};
		static int innerLowerEyePointsR[] = {
			3,	8,
			12, 8,
			3,	4,
			12, 12,
			3,	12
		};

		static int outerUpperEyePointsL[] = {
			3,	11,
			3,	13,
			3,	7
		};

		static int outerLowerEyePointsL[] = {
			3,	7,
			3,	9,
			3,	11
		};

		static int innerUpperEyePointsL[] = {
			3,	11,
			12, 9,
			3,	1,
			12, 5,
			3,	7
		};

		static int innerLowerEyePointsL[] = {
			3,	7,
			12, 7,
			3,	3,
			12, 11,
			3,	11
		};

		drawSpline2D(img, outerUpperEyePointsR, td->featurePoints2D, 3);
		drawSpline2D(img, outerLowerEyePointsR, td->featurePoints2D, 3);
		drawSpline2D(img, innerUpperEyePointsR, td->featurePoints2D, 5);
		drawSpline2D(img, innerLowerEyePointsR, td->featurePoints2D, 5);

		drawSpline2D(img, outerUpperEyePointsL, td->featurePoints2D, 3);
		drawSpline2D(img, outerLowerEyePointsL, td->featurePoints2D, 3);
		drawSpline2D(img, innerUpperEyePointsL, td->featurePoints2D, 5);
		drawSpline2D(img, innerLowerEyePointsL, td->featurePoints2D, 5);

		static int eyesPoints[] = {
			3,	2,
			3,	4,
			3,	8,
			3,	10,
			3,	12,
			3,	14,
			12, 6,
			12, 8,
			12, 10,
			12, 12
		};
		static int eyesPoints2[] = {
			3,	1,
			3,	3,
			3,	7,
			3,	9,
			3,	11,
			3,	13,
			12, 5,
			12, 7,
			12, 9,
			12, 11
		};

		drawPoints2D(img, eyesPoints, td->featurePoints2D, 10, td->eyeClosure[1] < 0.5f, td);
		drawPoints2D(img, eyesPoints2, td->featurePoints2D, 10, td->eyeClosure[0] < 0.5f, td);

		static int eyebrowLinesPointsR[] = {
			4,	6,
			14,	4,
			4,	4,
			14,	2,
			4,	2
		};
		drawSpline2D(img, eyebrowLinesPointsR, td->featurePoints2D, 5);

		static int eyebrowLinesPointsL[] = {
			4,	1,
			14,	1,
			4,	3,
			14,	3,
			4,	5
		};
		drawSpline2D(img, eyebrowLinesPointsL, td->featurePoints2D, 5);

		static int eyebrowPoints[] = {
			4,	1,
			4,	2,
			4,	3,
			4,	4,
			4,	5,
			4,	6,
			14,	1,
			14,	2,
			14,	3,
			14,	4
		};

		drawPoints2D(img, eyebrowPoints, td->featurePoints2D, 10, 0, td);

		if (!drawOnly2DFeatures)
		{
			drawFaceModelAxes(img, td);
			drawGaze(img, td, 255, 0, 0);
		}
}

void VisageDrawing::drawLogo(VsImage* img, VsImage* logo)
{
	float logoWidth;
	float logoHeight;
	
	//logo aspect
	float logoAspect = logo->width / (float)logo->height;

	logoWidth = img->width/4;
	logoHeight = logoWidth / logoAspect;
	
	IplImage* resizedLogo = cvCreateImage(cvSize(logoWidth,logoHeight),logo->depth,logo->nChannels);
	cvResize(logo,resizedLogo, CV_INTER_LINEAR);

	//set logo ROI diemensions and position in image
	CvRect logoROI;
	logoROI.x = img->width - logoWidth;
	logoROI.y = 0;
	logoROI.width = logoWidth;
	logoROI.height = logoHeight;

	//Set the ROI to the same size as the logo
	cvSetImageROI((IplImage*)img, logoROI);

	//Add the logo to the designated ROI
	int width = img->width;
	int height = resizedLogo->height;
	int nchannels = img->nChannels;
	int step = img->widthStep;
	int logoStep = resizedLogo->widthStep;

	uchar *data = (uchar*)img->imageData;
	uchar *dataLogo = (uchar*)resizedLogo->imageData;
	for (int i = 0; i < height; i++) {
		int k = 0;
		for (int j = 0; j < width; j++) {

			if (j % img->width >= logoROI.x)
			{
				int rImg = data[i*step + j*nchannels + 0];
				int gImg = data[i*step + j*nchannels + 1];
				int bImg = data[i*step + j*nchannels + 2];
				int aImg = data[i*step + j*nchannels + 3];
				//
				int rLogo = dataLogo[i*logoStep + k*nchannels + 0];
				int gLogo = dataLogo[i*logoStep + k*nchannels + 1];
				int bLogo = dataLogo[i*logoStep + k*nchannels + 2];
				int aLogo = dataLogo[i*logoStep + k*nchannels + 3];
				//
				k++;
				//Blend formula
				//dst[r] = src1[r] * alpha + src2[r]*(1-alpha)
				//alpha = src1[a]/255.0f
				float alpha = aLogo / 255.0f;
				//
				int rBlend = rLogo * alpha + rImg * (1.0f - alpha);
				int gBlend = gLogo * alpha + gImg * (1.0f - alpha);
				int bBlend = bLogo * alpha + bImg * (1.0f - alpha);
				data[i*step + j*nchannels + 0] = rBlend;
				data[i*step + j*nchannels + 1] = gBlend;
				data[i*step + j*nchannels + 2] = bBlend;
			}

		}
	}

	//reset ROI
	cvResetImageROI((IplImage*) img);
	//clean up
	cvReleaseImage(&resizedLogo);
}

void VisageDrawing::drawSpline2D(VsImage* img, int* points, FDP* fdp, int num)
{
	if (num < 2)
			return;
		vector<float> pointCoords;
		int n = 0;
		FeaturePoint* fp;
		for (int i = 0; i < num; i++) {
			fp = const_cast<FeaturePoint*>( &fdp->getFP(points[2*i],points[2*i+1]) );
			if(fp->defined)
				if(fp->pos[0]!=0 && fp->pos[1]!=0)
				{
					pointCoords.push_back(fp->pos[0]);
					pointCoords.push_back(fp->pos[1]);
					n++;
				}
		}

		if(pointCoords.size() == 0 || n <= 2)
			return;

		int factor = 10;
		vector<float> pointsToDraw;
		VisageDrawing::calcSpline(pointCoords, factor, pointsToDraw);
		int nVert = (int)pointsToDraw.size() / 2;
		float *vertices = new float[nVert*2];
		int cnt = 0;
		for (int i = 0; i < nVert; ++i)
		{
			vertices[2*i+0] = pointsToDraw.at(cnt++)*img->width;
			vertices[2*i+1] = (1 - pointsToDraw.at(cnt++))*img->height;
		}

		CvScalar color = cvScalar(222,196,176,160);

		for(int j = 0; j < nVert-1; j++)
		{
			cvLine(img, cvPoint(vertices[2 * j + 0], vertices[2 * j + 1]), cvPoint(vertices[2 * (j + 1) + 0], vertices[2 * (j + 1) + 1]), color, 1);
		}

		//clean-up
		delete[] vertices;
}

void VisageDrawing::drawPoints2D(VsImage* img, int* points, FDP* fdp, int num, bool singleColor, FaceData* td)
{
	float radius = MAX(td->faceScale * 0.02f, 2);
	FeaturePoint* fp;
	for (int i = 0; i < num; i++)
	{
		fp = const_cast<FeaturePoint*>( &fdp->getFP(points[2*i],points[2*i+1]) );
		if(fp->defined && fp->pos[0]!=0 && fp->pos[1]!=0)
		{
			float x = fp->pos[0] * img->width;
			float y = (1-fp->pos[1]) * img->height;
			CvScalar color = cvScalar(0, 0, 0, 255);
			cvCircle(img, cvPoint(x, y), cvRound(radius), color, -1);

			if(!singleColor)
			{
				float radius2 = MIN(cvRound(radius * 0.6f), radius - 1);

#if defined(ANDROID) || defined(MAC_OS_X) || defined(EMSCRIPTEN)
				CvScalar color = cvScalar(0, 255, 255, 255);
#else
				CvScalar color = cvScalar(255, 255, 0, 255);
#endif
				cvCircle(img, cvPoint(x, y), cvRound(radius2), color, -1);
			}
		}
	}
}

void VisageDrawing::drawFaceModelAxes(VsImage* img, FaceData* trackingData)
{
	//set projection
	int f = trackingData->cameraFocus;

	float x_offset = 1;
	float y_offset = 1;

	if (img->width > img->height)
		x_offset = (float)img->width / img->height;
	else if (img->width < img->height)
		y_offset = (float)img->height / img->width;

	float frustum_near = 0.001;
	float frustum_far = 30;
	float frustum_x = x_offset * frustum_near/f;
	float frustum_y = y_offset * frustum_near/f;

	float A = (frustum_x - frustum_x)/(frustum_x + frustum_x);
	float B = (frustum_y - frustum_y)/(frustum_y + frustum_y);
	float C = - ((frustum_far + frustum_near)/(frustum_far - frustum_near));
	float D = - ((2*frustum_near*frustum_far)/(frustum_far-frustum_near));
	float x1 = (2*frustum_near)/(frustum_x+frustum_x);
	float y2 = (2*frustum_near)/(frustum_y+frustum_y);
	float frustumMatrix[16] = { x1, 0,  0,  0,
								0,  y2, 0,  0,
								A,  B,  C, -1,
								0,  0,  D,  0 };
	float camera[3] = {0, 0, 10};
	float origin[3] = {0, 0, 0};
	float destX[3] = {1, 0, 0};
	float destY[3] = {0, -1, 0};
	float destZ[3] = {0, 0, 1};

	float sinrx = sin(-trackingData->faceRotation[0]);
	float sinry = sin(trackingData->faceRotation[1]);
	float sinrz = sin(-trackingData->faceRotation[2]);
	float cosrx = cos(trackingData->faceRotation[0]);
	float cosry = cos(trackingData->faceRotation[1]);
	float cosrz = cos(trackingData->faceRotation[2]);

	//set the rotation matrix
	float R00 = cosry*cosrz+sinrx*sinry*sinrz;
	float R01 = -cosry*sinrz+sinrx*sinry*cosrz;
	float R02 = cosrx*sinry;
	float R10 = cosrx*sinrz;
	float R11 = cosrx*cosrz;
	float R12 = -sinrx;
	float R20 = -sinry*cosrz+sinrx*cosry*sinrz;
	float R21 = sinry*sinrz+sinrx*cosry*cosrz;
	float R22 = cosrx*cosry;

	//apply rotation
	float dest_new_origin[3] = {0, 0, 0};
	dest_new_origin[0] = 1*(R00*origin[0]+R01*origin[1]+R02*origin[2]);
	dest_new_origin[1] = 1*(R10*origin[0]+R11*origin[1]+R12*origin[2]);
	dest_new_origin[2] = 1*(R20*origin[0]+R21*origin[1]+R22*origin[2]);

	float dest_new_x[3] = {0,0,0};
	dest_new_x[0] = 1*(R00*destX[0]+R01*destX[1]+R02*destX[2]);
	dest_new_x[1] = 1*(R10*destX[0]+R11*destX[1]+R12*destX[2]);
	dest_new_x[2] = 1*(R20*destX[0]+R21*destX[1]+R22*destX[2]);

	float dest_new_y[3] = {0,0,0};
	dest_new_y[0] = 1*(R00*destY[0]+R01*destY[1]+R02*destY[2]);
	dest_new_y[1] = 1*(R10*destY[0]+R11*destY[1]+R12*destY[2]);
	dest_new_y[2] = 1*(R20*destY[0]+R21*destY[1]+R22*destY[2]);

	float dest_new_z[3] = {0,0,0};
	dest_new_z[0] = 1*(R00*destZ[0]+R01*destZ[1]+R02*destZ[2]);
	dest_new_z[1] = 1*(R10*destZ[0]+R11*destZ[1]+R12*destZ[2]);
	dest_new_z[2] = 1*(R20*destZ[0]+R21*destZ[1]+R22*destZ[2]);

	//project on the screen
	float destPomOrigin[4] = {dest_new_origin[0], dest_new_origin[1], dest_new_origin[2] - camera[2], 1};
	float resultOrigin[4];
	multiplyMatrix(destPomOrigin, frustumMatrix, resultOrigin, 1, 4, 4);

	float destPomX[4] = {dest_new_x[0], dest_new_x[1], dest_new_x[2] - camera[2], 1};
	float resultX[4];
	multiplyMatrix(destPomX, frustumMatrix, resultX, 1, 4, 4);

	float destPomY[4] = {dest_new_y[0], dest_new_y[1], dest_new_y[2] - camera[2], 1};
	float resultY[4];
	multiplyMatrix(destPomY, frustumMatrix, resultY, 1, 4, 4);

	float destPomZ[4] = {dest_new_z[0], dest_new_z[1], dest_new_z[2] - camera[2], 1};
	float resultZ[4];
	multiplyMatrix(destPomZ, frustumMatrix, resultZ, 1, 4, 4);

	float dest[3] = {};
	float center_position[2] = {};

	FDP* featurePoints2D = trackingData->featurePoints2D;
	FeaturePoint* left_eyebrow = const_cast<FeaturePoint*>( &featurePoints2D->getFP(4,1));
	FeaturePoint* right_eyebrow = const_cast<FeaturePoint*>( &featurePoints2D->getFP(4,2));

	if (left_eyebrow->defined && right_eyebrow->defined){
		center_position[0] = (left_eyebrow->pos[0] +  right_eyebrow->pos[0])/2.0;
		center_position[1] = 1-((left_eyebrow->pos[1] +  right_eyebrow->pos[1])/2.0);
	}

	float center_pos[2] = {center_position[0]*img->width, center_position[1]*img->height};

	resultOrigin[0] = (resultOrigin[0]/resultOrigin[3] + 1)/2 * img->width;
	resultOrigin[1] = (resultOrigin[1]/resultOrigin[3] + 1)/2 * img->height;
	resultOrigin[2] = (resultOrigin[2]/resultOrigin[3] + 1)/2;

#if defined(ANDROID) || defined(MAC_OS_X) || defined(EMSCRIPTEN)
	CvScalar color_x = cvScalar(255,0,0,255);
	CvScalar color_y = cvScalar(0,255,0,255);
	CvScalar color_z = cvScalar(0,0,255,255);
#else
	CvScalar color_x = cvScalar(0,0,255,255);
	CvScalar color_y = cvScalar(0,255,0,255);
	CvScalar color_z = cvScalar(255,0,0,255);
#endif


	//draw x axis
	dest[0] = (resultX[0]/resultX[3] + 1)/2 * img->width;
	dest[1] = (resultX[1]/resultX[3] + 1)/2 * img->height;
	dest[2] = (resultX[2]/resultX[3] + 1)/2;

	cvLine(img, cvPoint(center_pos[0],center_pos[1]), cvPoint(center_pos[0] + (dest[0] - resultOrigin[0]),center_pos[1] + (dest[1] - resultOrigin[1])), color_x, 1);

	//draw y axis
	dest[0] = (resultY[0] / resultY[3] + 1) / 2 * img->width;
	dest[1] = (resultY[1] / resultY[3] + 1) / 2 * img->height;
	dest[2] = (resultY[2] / resultY[3] + 1) / 2;

	cvLine(img, cvPoint(center_pos[0],center_pos[1]), cvPoint(center_pos[0] + (dest[0] - resultOrigin[0]),center_pos[1] + (dest[1] - resultOrigin[1])), color_y, 1);

	//draw z axiss
	dest[0] = (resultZ[0] / resultZ[3] + 1) / 2 * img->width;
	dest[1] = (resultZ[1] / resultZ[3] + 1) / 2 * img->height;
	dest[2] = (resultZ[2] / resultZ[3] + 1) / 2;

	cvLine(img, cvPoint(center_pos[0],center_pos[1]), cvPoint(center_pos[0] + (dest[0] - resultOrigin[0]),center_pos[1] + (dest[1] - resultOrigin[1])), color_z, 1);
}

void VisageDrawing::drawGaze(VsImage* img, FaceData* td, int r, int g, int b)
{
	int f = td->cameraFocus;
	float x_offset = 1.00f;
	float y_offset = 1.00f;

	int m_width = img->width;
	int m_height = img->height;

	if (m_width >= m_height)
		x_offset = (float)m_width  / m_height;
	else
		y_offset = (float)m_height / m_width ;

	float frustum_near = 0.001f;
	int frustum_far = 30;
	float frustum_x = x_offset*frustum_near/f;
	float frustum_y = y_offset*frustum_near/f;

	float A = (frustum_x - frustum_x)/(frustum_x + frustum_x);
	float B = (frustum_y - frustum_y)/(frustum_y + frustum_y);
	float C = - ((frustum_far + frustum_near)/(frustum_far - frustum_near));
	float D = - ((2*frustum_near*frustum_far)/(frustum_far-frustum_near));
	float x1 = (2*frustum_near)/(frustum_x+frustum_x);
	float y2 = (2*frustum_near)/(frustum_y+frustum_y);
	float frustumMatrix[16] = { x1, 0,  0,  0,
								0,  y2, 0,  0,
								A,  B,  C, -1,
								0,  0,  D,  0 };

	float dest[] = {0, 0, 1};
	float camera[3] = {0, 0, 10};
	float origin[3] = {0, 0, 0};

	float sinrx = sinf(td->gazeDirectionGlobal[0]);
	float sinry = sinf(td->gazeDirectionGlobal[1]);
	float sinrz = sinf(td->gazeDirectionGlobal[2]);
	float cosrx = cosf(td->gazeDirectionGlobal[0]);
	float cosry = cosf(td->gazeDirectionGlobal[1]);
	float cosrz = cosf(td->gazeDirectionGlobal[2]);

	//set the rotation matrix
	float R00 = cosry*cosrz+sinrx*sinry*sinrz;
	float R01 = -cosry*sinrz+sinrx*sinry*cosrz;
	float R02 = cosrx*sinry;
	float R10 = cosrx*sinrz;
	float R11 = cosrx*cosrz;
	float R12 = -sinrx;
	float R20 = -sinry*cosrz+sinrx*cosry*sinrz;
	float R21 = sinry*sinrz+sinrx*cosry*cosrz;
	float R22 = cosrx*cosry;

	//apply rotation
	float dest_new_origin[3] = {0, 0, 0};
	dest_new_origin[0] = 1*(R00*origin[0]+R01*origin[1]+R02*origin[2]);
	dest_new_origin[1] = 1*(R10*origin[0]+R11*origin[1]+R12*origin[2]);
	dest_new_origin[2] = 1*(R20*origin[0]+R21*origin[1]+R22*origin[2]);

	float dest_new[3] = {0, 0, 0};
	dest_new[0] = 1*(R00*dest[0]+R01*dest[1]+R02*dest[2]);
	dest_new[1] = 1*(R10*dest[0]+R11*dest[1]+R12*dest[2]);
	dest_new[2] = 1*(R20*dest[0]+R21*dest[1]+R22*dest[2]);

	//project on the screen
	float destPomOrigin[4] = {dest_new_origin[0], dest_new_origin[1], dest_new_origin[2] - camera[2], 1};
	float resultOrigin[4];
	multiplyMatrix(destPomOrigin, frustumMatrix, resultOrigin, 1, 4, 4);

	float destPom[4] = {dest_new[0], dest_new[1], dest_new[2] - camera[2], 1};
	float result[4];
	multiplyMatrix(destPom, frustumMatrix, result, 1, 4, 4);


	resultOrigin[0] = (resultOrigin[0]/resultOrigin[3] + 1)/2*m_width;
	resultOrigin[1] = (resultOrigin[1]/resultOrigin[3] + 1)/2*m_height;
	resultOrigin[2] = (resultOrigin[2]/resultOrigin[3] + 1)/2;

	result[0] = (result[0]/result[3] + 1)/2*m_width;
	result[1] = (result[1]/result[3] + 1)/2*m_height;
	result[2] = (result[2]/result[3] + 1)/2;

	FDP* featurePoints2D = td->featurePoints2D;
	FeaturePoint* le = const_cast<FeaturePoint*>( &featurePoints2D->getFP(3,5));
	FeaturePoint* re = const_cast<FeaturePoint*>( &featurePoints2D->getFP(3,6));


#if defined(ANDROID) || defined(MAC_OS_X) || defined(EMSCRIPTEN)
	CvScalar color = cvScalar(r, g, b);
#else
	CvScalar color = cvScalar(b, g, r);
#endif

	if (le->defined && td->eyeClosure[0] && (td->eyeClosure[0] > 0.5f))
	{
		//apply translation left eye
		float left_gaze_x_start = le->pos[0]*m_width;
		float left_gaze_y_start = le->pos[1]*m_height;

		//draw left eye gaze
		cvLine(img, cvPoint(left_gaze_x_start, (m_height - left_gaze_y_start)), cvPoint(left_gaze_x_start + (result[0] - resultOrigin[0]), (m_height - (left_gaze_y_start + (result[1] - resultOrigin[1])))), color, 2);
	}
	if (re->defined && td->eyeClosure[1] && (td->eyeClosure[1] > 0.5f))
	{
		//apply translation right eye
		float right_gaze_x_start = re->pos[0]*m_width;
		float right_gaze_y_start = re->pos[1]*m_height;

		//draw righ eye gaze
		cvLine(img, cvPoint(right_gaze_x_start, (m_height - right_gaze_y_start)), cvPoint(right_gaze_x_start + (result[0] - resultOrigin[0]), (m_height - (right_gaze_y_start + (result[1] - resultOrigin[1])))), color, 2);
	}
}

void VisageDrawing::drawLines(VsImage* img, int* points, FDP* fdp, int num, int r, int g, int b)
{
	float* vertices = new float[2 * num];
	int faces = 1;

	for(int i = 0; i < faces; i++)
	{
		FeaturePoint* fp;
		int n = 0;
		for(int j = 0; j < num; j++)
		{
			fp = const_cast<FeaturePoint*>(&fdp->getFP(points[2 * j], points[2 * j + 1]));
			if(fp->defined)
			{
				if(fp->pos[0] != 0 && fp->pos[1] != 0)
				{
					vertices[2 * n + 0] = fp->pos[0] * img->width;
					vertices[2 * n + 1] = (1-fp->pos[1]) * img->height;
					n++;
				}
			}
		}

		CvScalar color = cvScalar(b, g, r);

		for(int j = 0; j < n ; j+=2)
		{
			cvLine(img, cvPoint(vertices[2 * j + 0], vertices[2 * j + 1]), cvPoint(vertices[2 * (j + 1) + 0], vertices[2 * (j + 1) + 1]), color, 1);
		}
	}

	delete[] vertices;
}

void VisageDrawing::drawGender(IplImage* frame, const FaceData &faceData, int gender, bool writeText)
{
	FeaturePoint head = faceData.featurePoints2D->getFP(12, 1);

	if (!head.defined)
		return;

	float x = 0.0f;
	float y = 0.0f;
	float radius = 0.0f;

	const FeaturePoint &nose1 = faceData.featurePoints2D->getFP(9, 1);
	const FeaturePoint &nose2 = faceData.featurePoints2D->getFP(9, 2);

	if (!nose1.defined || !nose2.defined)
	{
		x = head.pos[0] * frame->width;
		y = (1 - head.pos[1]) * frame->height;
		radius = faceData.faceScale / 1.7f;
	}
	else
	{
		x = (nose1.pos[0] * frame->width + nose2.pos[0] * frame->width) / 2.0f;
		y = ((1 - nose1.pos[1]) * frame->height + (1 - nose2.pos[1]) * frame->height) / 2.0f;
		radius = faceData.faceScale / 1.7f;
	}

	cvCircle(frame, cvPoint(x, y), radius, gender ? CV_RGB(0, 0, 255) : CV_RGB(255, 0, 255), 2);

	if (writeText)
	{
		CvFont font, outline;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 1, 8);
		cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 3, 8);

		//string gender_txt = gender ? "Male" : "Female";
		//cvPutText(frame, gender_txt.c_str(), cvPoint(x - (gender ? 14 : 22), y), &outline, CV_RGB(0,0,0));
		//cvPutText(frame, gender_txt.c_str(), cvPoint(x - (gender ? 14 : 22), y), &font, gender ? CV_RGB(0,128,255) : CV_RGB(255,0,255));
	}
}

void VisageDrawing::drawAge(IplImage* frame, const FaceData &faceData, int age)
{
	CvFont font;
	CvFont outline;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 1, 8);
	cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 2, 8);

	ostringstream intToString;
	intToString << age;
	string age_txt = intToString.str();


	FeaturePoint head = faceData.featurePoints2D->getFP(4, 2);

	if (head.defined)
	{
		cvPutText(frame, age_txt.c_str(), cvPoint(head.pos[0] * frame->width - faceData.faceScale / 1.7f + faceData.faceScale*1.176f / 2, (1 - head.pos[1])*frame->height - faceData.faceScale / 1.7f + faceData.faceScale*1.176f / 2), &outline, CV_RGB(0, 255, 0));
	}
}

void VisageDrawing::drawEmotions(IplImage* frame, const FaceData &faceData, float* estimates)
{
	int x = 20;
	int y = frame->height - 100;
	int verticalStep = 12;
	int emotionsBarOffset = 70;

	float estimates_[8] = { 0 };

	estimates_[0] = 1.0f;

	for (int j = 0; j < 7; j++) estimates_[j + 1] = estimates[j];

	CvFont font;
	CvFont outline;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 1, 8);
	cvInitFont(&outline, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 3, 8);

	string emotions[8] = { "Emotion", "Anger", "Disgust", "Fear", "Happiness", "Sadness", "Surprise", "Neutral" };

	for (int j = 0; j < 8; j++)
	{
		cvPutText(frame, emotions[j].c_str(), cvPoint(x, y + j * verticalStep), &outline, CV_RGB(0, 0, 0));
	}

	for (int j = 0; j < 8; j++)
	{
		int length = estimates_[j] * 100;
		cvPutText(frame, emotions[j].c_str(), cvPoint(x, y + j * verticalStep), &font, (j == 0) ? CV_RGB(255, 255, 255) : CV_RGB(0, 255, 0));
		cvLine(frame, cvPoint(x + emotionsBarOffset, y - 3 + j * verticalStep), cvPoint(x + emotionsBarOffset + length, y - 3 + j * verticalStep), CV_RGB(0, 0, 0), 6);
		cvLine(frame, cvPoint(x + emotionsBarOffset, y - 3 + j * verticalStep), cvPoint(x + emotionsBarOffset + length, y - 3 + j * verticalStep), (j == 0) ? CV_RGB(255, 255, 255) : CV_RGB(0, 255, 0), 4);
	}

	FeaturePoint head = faceData.featurePoints2D->getFP(12, 1);

	if (head.defined)
	{
		cvRectangleR(frame, cvRect(head.pos[0] * frame->width - faceData.faceScale / 1.7f, (1 - head.pos[1])*frame->height - faceData.faceScale / 1.7f, faceData.faceScale*1.176f, faceData.faceScale*1.176f), cvScalar(0, 255, 0));
	}
}

void VisageDrawing::drawMatchingIdentity(IplImage* frame, const FaceData &faceData, const char* matchName)
{
	CvFont font;

	float font_size = faceData.faceScale < 185 ? 0.4 : 0.6;

	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, font_size, font_size, 0, 1, 8);

	float x = faceData.featurePoints2D->getFP(12, 1).pos[0] * frame->width;
	float y = frame->height - faceData.featurePoints2D->getFP(12, 1).pos[1] * frame->height;
	int scale = faceData.faceScale;

	cvPutText(frame, matchName, cvPoint(x - 0.6*scale, y + scale - 5), &font, cvScalar(0, 255, 0, 0));
	cvRectangle(frame, cvPoint(x - 0.6*scale, y + scale), cvPoint(x + 0.6*scale, y - scale), cvScalar(0, 255, 0, 0), 1, 8, 0);
}

void VisageDrawing::multiplyMatrix(float* m1, float* m2, float* res, int M, int N, int P)
{
	for (int i = 0; i < M; ++i)
	{
		for (int j = 0; j < P; ++j)
		{
			float sum = 0;
			for (int k = 0; k < N; ++k)
			{
				sum += m1[i*N+k] * m2[k*P+j];
			}
			res[i*P+j] = sum;
		}
	}
}

void VisageDrawing::calcSpline(vector <float>& inputPoints, int ratio, vector <float>& outputPoints)
{

	int nPoints, nPointsToDraw, nLines;


	nPoints = (int)inputPoints.size()/2 + 2;
	nPointsToDraw = (int)inputPoints.size()/2 + ((int)inputPoints.size()/2 - 1) * ratio;
	nLines = nPoints - 1 - 2;

	//adding fake point's coordinates
	inputPoints.insert(inputPoints.begin(), inputPoints[1] + (inputPoints[1] - inputPoints[3]));
	inputPoints.insert(inputPoints.begin(), inputPoints[1] + (inputPoints[1] - inputPoints[3]));
	inputPoints.insert(inputPoints.end(), inputPoints[inputPoints.size()/2-2] + (inputPoints[inputPoints.size()/2-2] - inputPoints[inputPoints.size()/2-4]));
	inputPoints.insert(inputPoints.end(), inputPoints[inputPoints.size()/2-1] + (inputPoints[inputPoints.size()/2-1] - inputPoints[inputPoints.size()/2-3]));


	//calculate spline

	Vec2D p0(0,0), p1(0,0), p2(0,0), p3(0,0);
	CubicPoly px, py;


	outputPoints.resize(2*nPointsToDraw);

	for(int i = 0; i < nPoints - 2; i++) {
			outputPoints[i*2*(ratio+1)] = inputPoints[2*i+2];
			outputPoints[i*2*(ratio+1)+1] = inputPoints[2*i+1+2];
	}

	for(int i = 0; i < 2*nLines; i=i+2) {
		p0.x = inputPoints[i];
		p0.y = inputPoints[i+1];
		p1.x = inputPoints[i+2];
		p1.y = inputPoints[i+3];
		p2.x = inputPoints[i+4];
		p2.y = inputPoints[i+5];
		p3.x = inputPoints[i+6];
		p3.y = inputPoints[i+7];

		InitCentripetalCR(p0, p1, p2, p3, px, py);


		for(int j = 1; j <= ratio; j++) {
			outputPoints[i*(ratio+1)+2*j] = (px.eval(1.00f/(ratio+1)*(j)));
			outputPoints[i*(ratio+1)+2*j+1] =(py.eval(1.00f/(ratio+1)*(j)));
		}

	}

	//deleting fake points
	inputPoints.erase(inputPoints.begin(), inputPoints.begin()+2);
	inputPoints.erase(inputPoints.end()-2, inputPoints.end());

//	int i = 0;

}

void VisageDrawing::InitCentripetalCR(const Vec2D& p0, const Vec2D& p1, const Vec2D& p2, const Vec2D& p3, CubicPoly &px, CubicPoly &py)
{
	float dt0 = powf(VecDistSquared(p0, p1), 0.25f);
	float dt1 = powf(VecDistSquared(p1, p2), 0.25f);
	float dt2 = powf(VecDistSquared(p2, p3), 0.25f);

	// safety check for repeated points
	if (dt1 < 1e-4f)    dt1 = 1.0f;
	if (dt0 < 1e-4f)    dt0 = dt1;
	if (dt2 < 1e-4f)    dt2 = dt1;

	InitNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, px);
	InitNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, py);
}

float VisageDrawing::VecDistSquared(const Vec2D& p, const Vec2D& q)
{
	float dx = q.x - p.x;
	float dy = q.y - p.y;
	return dx*dx + dy*dy;
}

void VisageDrawing::InitNonuniformCatmullRom(float x0, float x1, float x2, float x3, float dt0, float dt1, float dt2, CubicPoly &p)
{
	// compute tangents when parameterized in [t1,t2]
	float t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
	float t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;

	// rescale tangents for parametrization in [0,1]
	t1 *= dt1;
	t2 *= dt1;

	InitCubicPoly(x1, x2, t1, t2, p);
}

void VisageDrawing::InitCubicPoly(float x0, float x1, float t0, float t1, CubicPoly &p)
{
	p.c0 = x0;
	p.c1 = t0;
	p.c2 = -3*x0 + 3*x1 - 2*t0 - t1;
	p.c3 = 2*x0 - 2*x1 + t0 + t1;
}
}

