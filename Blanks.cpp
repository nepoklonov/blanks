
#include "stdafx.h"

#include <cv.h>
#include <highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using namespace std;

CvSeq* seqMarker = 0;

#define CV_PIXEL(type,img,x,y) (((type*)((img)->imageData+(y)*(img)->widthStep))+(x)*(img)->nChannels)
const CvScalar RED = CV_RGB(255, 0, 0);
const CvScalar ORANGE = CV_RGB(255, 100, 0);
const CvScalar YELLOW = CV_RGB(255, 255, 0);
const CvScalar GREEN = CV_RGB(0, 255, 0);
const CvScalar BLUE = CV_RGB(0, 0, 255);
const CvScalar VIOLET = CV_RGB(255, 0, 255);
const CvScalar BLACK = CV_RGB(0, 0, 0);
const CvScalar WHITE = CV_RGB(255, 255, 255);


int cvSeqCompSquare(const void * a, const void * b)
{

	if (*(CvSeq**)a == 0 || *(CvSeq**)b == 0) {
		return *(CvSeq**)a == 0 ? -1 : 1;
	}
	double d = cvMatchShapes(*(CvSeq**)b, seqMarker, 3) -
		cvMatchShapes(*(CvSeq**)a, seqMarker, 3);
	return d > 0 ? 1 : -1;
}

int cvSeqCompSqArea(const void * a, const void * b)
{
	if (*(CvSeq**)a == 0 || *(CvSeq**)b == 0) {
		return *(CvSeq**)a == 0 ? -1 : 1;
	}
	double d = cvContourArea(*(CvSeq**)a) - cvContourArea(*(CvSeq**)b);
	return d > 0 ? 1 : -1;
}

double rectDist(CvBox2D p0, CvBox2D p1) {
	double x0 = p0.center.x;
	double x1 = p1.center.x;
	double y0 = p0.center.y;
	double y1 = p1.center.y;
	return sqrt(pow((x0 - x1), 2) + pow((y0 - y1), 2));
}

int * findBoundPixels(IplImage * image, CvSeq * seq0) {
	CvRect rect = cvBoundingRect(seq0);
	int zmin = 255;
	int zmiddle = 0;
	for (int x = rect.x - rect.width / 2; x < rect.x + rect.width / 2; x++) {
		for (int y = rect.y - rect.height / 2; y < rect.y + rect.height / 2; y++) {
			if (x > 0 && y > 0 && x < image->width && y < image->height) {
				uchar* ptr = (uchar*)(image->imageData + y * image->widthStep);
				int pix = 0;
				pix += (ptr[3 * x]);
				pix += (ptr[3 * x + 1]);
				pix += (ptr[3 * x + 2]);
				pix /= 3;
				zmin = min(zmin, pix);
				zmiddle += pix;
			}
		}
	}
	zmiddle /= (rect.width * rect.height);
	int result[] = { zmin, zmiddle };
	return result;
}

void findMarkers(CvSeq* markers[], CvSeq * seqM[], CvMemStorage * storage, IplImage * rgb) {
	for (int i = 1; i < 6; i++) {
		CvBox2D p0 = cvMinAreaRect2(seqM[i], storage);
		for (int j = i + 1; j < 6; j++) {
			CvBox2D p1 = cvMinAreaRect2(seqM[j], storage);
			for (int k = j + 1; k < 6; k++) {
				CvBox2D p2 = cvMinAreaRect2(seqM[k], storage);
				double a = rectDist(p0, p1);
				double b = rectDist(p1, p2);
				double c = rectDist(p2, p0);
				double mx = max(max(a, b), c);
				double mn = min(min(a, b), c);
				double md = a + b + c - mx - mn;
				double angle = acos((a * a + b * b + c * c - 2 * mx * mx)
					/ (2 * a * b * c / mx)) * 180 / CV_PI;
				if (angle > 170) {
					markers[4] = abs(mx - a) < 0.001 ? seqM[k] : abs(mx - b) < 0.001 ? seqM[i] : seqM[j];
					markers[2] = abs(mn - a) < 0.001 ? seqM[k] : abs(mn - b) < 0.001 ? seqM[i] : seqM[j];
					markers[3] = abs(md - a) < 0.001 ? seqM[k] : abs(md - b) < 0.001 ? seqM[i] : seqM[j];
				}
			}
		}
	}
	double fDist = 0, sDist = 0;
	for (int i = 1; i < 6; i++) {
		double dist02 = rectDist(cvMinAreaRect2(markers[2], storage), cvMinAreaRect2(seqM[i], storage));
		double dist13 = rectDist(cvMinAreaRect2(markers[3], storage), cvMinAreaRect2(seqM[i], storage));
		if (dist02 > fDist) {
			fDist = dist02;
			markers[0] = seqM[i];
		}
		if (dist13 > sDist) {
			sDist = dist13;
			markers[1] = seqM[i];
		}
	}
}


int countColorR(IplImage * image, CvRect rect, int zmin, int zmiddle) {
	int z = 0;
	for (int x = rect.x - rect.width / 2; x < rect.x + rect.width / 2; x++) {
		for (int y = rect.y - rect.height / 2; y < rect.y + rect.height / 2; y++) {
			if (x > 0 && y > 0 && x < image->width && y < image->height) {
				uchar* ptr = (uchar*)(image->imageData + y * image->widthStep);
				int pix = 0;
				pix += (ptr[3 * x]);
				pix += (ptr[3 * x + 1]);
				pix += (ptr[3 * x + 2]);
				pix /= 3;
				if (pix < (255 * 4 + zmiddle) / 5) {
					z++;
				}
				if (pix < (zmiddle + 255 * 3) / 4) {
					z += 2;
				}
				if (pix < (zmiddle + 255 * 2) / 3) {
					z += 4;
				}
				if (pix < (zmiddle + 255) / 2) {
					z += 16;
				}
				if (pix < (zmiddle + zmin) / 2) {
					z += 625;
				}
				if (pix < (zmiddle + zmin * 2) / 3) {
					z += 3125;
				}
				if (pix < (zmiddle + zmin * 4) / 5) {
					z += 10000;
				}
				if (pix < (zmiddle + zmin * 8) / 9) {
					z += 31250;
				}
			}
		}
	}
	return z;
}

int countColor(IplImage * image, CvSeq * seq0, int zmin, int zmiddle) {
	CvRect rect = cvBoundingRect(seq0);
	int result = countColorR(image, rect, zmin, zmiddle);
	return result;
}

int * getMainMarkerParams(CvMemStorage * storage, CvSeq * markers[]) {
	double al = 0;
	double r = 0;
	for (int i = 0; i < 5; i++) {
		r += cvMinAreaRect2(markers[i], storage).size.width +
			cvMinAreaRect2(markers[i], storage).size.height;
		al += cvMinAreaRect2(markers[i], storage).angle;
	}
	al /= 5;
	r /= 10;
	int result[] = { al, r };
	return result;
}

void testMatch(IplImage* original, IplImage* marker)
{
	assert(original != 0);
	assert(marker != 0);

	IplImage *src = 0, *dst = 0;

	src = cvCloneImage(original);
	cvSmooth(src, src, CV_GAUSSIAN, 3, 3);

	IplImage* binI = cvCreateImage(cvGetSize(src), 8, 1);
	IplImage* binMarker = cvCreateImage(cvGetSize(marker), 8, 1);

	IplImage* rgb = cvCreateImage(cvGetSize(original), 8, 3);
	IplImage* rgbcopy = cvCreateImage(cvGetSize(original), 8, 3);
	IplImage* rotated = cvCreateImage(cvGetSize(original), 8, 3);
	cvConvertImage(src, rgb, CV_GRAY2BGR);

	IplImage* rgbMarker = cvCreateImage(cvGetSize(marker), 8, 3);
	cvConvertImage(marker, rgbMarker, CV_GRAY2BGR);

	cvCanny(src, binI, 50, 200);
	cvCanny(marker, binMarker, 50, 200);

	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contoursI = 0, *contoursMarker = 0;

	int contoursCont = cvFindContours(binI, storage, &contoursI, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0);

	if (contoursI != 0) {
		for (CvSeq* seq0 = contoursI; seq0 != 0; seq0 = seq0->h_next) {
			cvDrawContours(rgb, seq0, CV_RGB(255, 216, 0), CV_RGB(0, 0, 250), 0, 1, 8);
		}
	}
	cvNamedWindow("cont", 1);
	cvShowImage("cont", rgb);

	cvConvertImage(src, rgb, CV_GRAY2BGR);
	rgbcopy = cvCloneImage(rgb);

	cvFindContours(binMarker, storage, &contoursMarker, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	double perimMarker = 0;

	if (contoursMarker != 0) {
		for (CvSeq* seq0 = contoursMarker; seq0 != 0; seq0 = seq0->h_next) {
			double perim = cvContourPerimeter(seq0);
			if (perim>perimMarker) {
				perimMarker = perim;
				seqMarker = seq0;
			}
			cvDrawContours(rgbMarker, seq0, CV_RGB(255, 216, 0), CV_RGB(0, 0, 250), 0, 1, 8);
		}
	}
	cvDrawContours(rgbMarker, seqMarker, CV_RGB(52, 201, 36), CV_RGB(36, 201, 197), 0, 2, 8);
	cvNamedWindow("contT", 1);
	cvShowImage("contT", rgbMarker);

	CvSeq* seqM[] = { 0, 0, 0, 0, 0, 0 };
	//assert(contoursI != 0);
	for (CvSeq* seq0 = contoursI; seq0 != 0; seq0 = seq0->h_next) {
		bool flag = false;
		if (cvMatchShapes(seq0, seqMarker, CV_CONTOURS_MATCH_I3) < 0.01) {
			int * zs = findBoundPixels(rgb, seq0);
			int z = countColor(rgb, seq0, zs[0], zs[1]);
			if (z > 100) {
				for (int i = 1; i < 6; i++) {
					if (seqM[i] == 0) {
						continue;
					}
					double x0 = cvBoundingRect(seq0).x;
					double x1 = cvBoundingRect(seqM[i]).x;
					double y0 = cvBoundingRect(seq0).y;
					double y1 = cvBoundingRect(seqM[i]).y;
					double w0 = cvBoundingRect(seq0).width;
					double w1 = cvBoundingRect(seqM[i]).width;
					double h0 = cvBoundingRect(seq0).height;
					double h1 = cvBoundingRect(seqM[i]).height;
					flag |= (sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)) <
						min(sqrt(w0 * w0 + h0 * h0), sqrt(w1 * w1 + h1 * h1)) / 2);
				}
				if (!flag) {
					seqM[0] = seq0;
					qsort(seqM, 6, sizeof(CvSeq*), cvSeqCompSqArea);
				}
			}
		}
	}
	CvSeq * markers[] = { 0, 0, 0, 0, 0 };
	findMarkers(markers, seqM, storage, rgb);
	cvDrawContours(rgb, markers[0], RED, RED, 0);
	cvDrawContours(rgb, markers[1], ORANGE, ORANGE, 0);
	cvDrawContours(rgb, markers[2], YELLOW, YELLOW, 0);
	cvDrawContours(rgb, markers[3], GREEN, GREEN, 0);
	cvDrawContours(rgb, markers[4], BLUE, BLUE, 0);
	//отклонение от вертикали
	int * mainSquareParams = getMainMarkerParams(storage, markers);
	CvPoint2D32f center = cvPoint2D32f(rgb->width / 2, rgb->height / 2);
	double angleR = mainSquareParams[0];
	CvPoint2D32f srcTri[3], dstTri[3];
	CvMat* rot_mat = cvCreateMat(2, 3, CV_32FC1);
	CvMat* warp_mat = cvCreateMat(2, 3, CV_32FC1);
	cv2DRotationMatrix(center, angleR, 1, rot_mat);
	cvWarpAffine(rgbcopy, rotated, rot_mat);
	cvCanny(rotated, binI, 50, 200);
	CvMemStorage* storageRotated = cvCreateMemStorage(0);
	contoursCont = cvFindContours(binI, storageRotated, &contoursI, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	cvFindContours(binMarker, storageRotated, &contoursMarker, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	perimMarker = 0;
	if (contoursMarker != 0) {
		for (CvSeq* seq0 = contoursMarker; seq0 != 0; seq0 = seq0->h_next) {
			double perim = cvContourPerimeter(seq0);
			if (perim > perimMarker) {
				perimMarker = perim;
				seqMarker = seq0;
			}
		}
	}
	//assert(contoursI != 0);
	for (int i = 0; i < sizeof(seqM); i++) {
		seqM[i] = 0;
	}
	for (CvSeq* seq0 = contoursI; seq0 != 0; seq0 = seq0->h_next) {
		bool flag = false;
		if (cvMatchShapes(seq0, seqMarker, CV_CONTOURS_MATCH_I3) < 0.01) {
			int * zs = findBoundPixels(rotated, seq0);
			int z = countColor(rotated, seq0, zs[0], zs[1]);
			if (z >= 100) {
				for (int i = 1; i < 6; i++) {
					if (seqM[i] == 0) {
						continue;
					}
					double x0 = cvBoundingRect(seq0).x;
					double x1 = cvBoundingRect(seqM[i]).x;
					double y0 = cvBoundingRect(seq0).y;
					double y1 = cvBoundingRect(seqM[i]).y;
					double w0 = cvBoundingRect(seq0).width;
					double w1 = cvBoundingRect(seqM[i]).width;
					double h0 = cvBoundingRect(seq0).height;
					double h1 = cvBoundingRect(seqM[i]).height;
					flag |= (sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)) <
						min(sqrt(w0 * w0 + h0 * h0), sqrt(w1 * w1 + h1 * h1)) / 2);
				}
				if (!flag) {
					seqM[0] = seq0;
					qsort(seqM, 6, sizeof(CvSeq*), cvSeqCompSqArea);
				}
			}
		}
	}

	findMarkers(markers, seqM, storageRotated, rotated);
	cvDrawContours(rotated, markers[0], RED, RED, 0);
	cvDrawContours(rotated, markers[1], ORANGE, ORANGE, 0);
	cvDrawContours(rotated, markers[2], YELLOW, YELLOW, 0);
	cvDrawContours(rotated, markers[3], GREEN, GREEN, 0);
	cvDrawContours(rotated, markers[4], BLUE, BLUE, 0);

	
	CvPoint2D32f center0 = cvMinAreaRect2(markers[0], storage).center;
	CvPoint2D32f center1 = cvMinAreaRect2(markers[1], storage).center;
	double cx0 = center0.x + mainSquareParams[1];
	double cx1 = center1.x - mainSquareParams[1];
	double cy = center0.y + center1.y;
	cy /= 2;
	double cdx = cx1 - cx0;
	
	int zmiddle = 0;
	int zmin = 255;
	int zsum = 0;
	for (int i = 0; i < 5; i++) {
		for (int x = cx0 + cdx / 5 * i; x < cx0 + cdx / 5 * (i + 1); x++) {
			for (int y = cy - mainSquareParams[1] / 2; y < cy + mainSquareParams[1] / 2; y++) {
				uchar* ptr = (uchar*)(rotated->imageData + y * rotated->widthStep);
				int pix = 0;
				pix += (ptr[3 * x]);
				pix += (ptr[3 * x + 1]);
				pix += (ptr[3 * x + 2]);
				pix /= 3;
				zmin = min(zmin, pix);
				zmiddle += pix;
			}
		}
	}
	zmiddle /= (5 * (cdx / 5) * mainSquareParams[1]);
	
	for (int i = 0; i < 5; i++) {
		CvRect rect = cvRect(cx0 + cdx / 5 * i + cdx / 10, cy, cdx / 5, mainSquareParams[1] / 2);
		int z = countColorR(rotated, rect, zmin, zmiddle);
		if (z / (5 * (cdx / 5) * mainSquareParams[1]) > 10) {
			cout << "@ ";
		}
		else {
			cout << "_ ";
		}
	}
	cout << "\n";
	
	
	CvPoint2D32f center2 = cvMinAreaRect2(markers[2], storage).center;
	CvPoint2D32f center3 = cvMinAreaRect2(markers[3], storage).center;
	double cybottom = center2.y + center3.y;
	cybottom /= 2;
	int cdy = cybottom - cy - mainSquareParams[1];
	int amount = 13;
	cdy /= amount;
	//
	for (int k = 1; k < amount + 1; k++) {
		double cx0 = center0.x + mainSquareParams[1] * 4 / 5;
		double cx1 = center1.x - mainSquareParams[1] * 4 / 5;
		double cy = center0.y + center1.y - mainSquareParams[1] / 4;
		cy /= 2;
		cy += k * cdy;
		double cdx = cx1 - cx0 + mainSquareParams[1] / 2;
		int zmiddle = 0;
		int zmin = 255;
		int zsum = 0;

		double skew = center2.x - center3.x - center1.x + center0.x;
		skew /= amount;
		skew /= 5;

		for (int i = 0; i < 5; i++) {
			int xxxx = cx0 + cdx / 5 * i + int(skew * (i - 1) * (k - 1)) + cx0 + cdx / 5 * (i + 1) + int(skew * (i) * (k));
			xxxx /= 2;
			int yyyy = cy;
			cvDrawCircle(rotated, cvPoint(xxxx, yyyy), 1, VIOLET, 1, 8, 0);
			for (int x = cx0 + cdx / 5 * i + int(skew * (i) * (k)); x < cx0 + cdx / 5 * (i + 1) + int(skew * (i + 1) * (k)); x++) {
				for (int y = cy - mainSquareParams[1] / 2; y < cy + mainSquareParams[1] / 2; y++) {
					uchar* ptr = (uchar*)(rotated->imageData + y * rotated->widthStep);
					int pix = 0;
					pix += (ptr[3 * x]);
					pix += (ptr[3 * x + 1]);
					pix += (ptr[3 * x + 2]);
					pix /= 3;
					zmin = min(zmin, pix);
					zmiddle += pix;
				}
			}
		}
		zmiddle /= (5 * (cdx / 5) * mainSquareParams[1]);
		//cout << zmin << " " << zmiddle;

		for (int i = 0; i < 5; i++) {
			CvRect rect = cvRect(cx0 + cdx / 5 * i + cdx / 10, cy, cdx / 5 / 2, mainSquareParams[1] / 2 / 2);
			int z = countColorR(rotated, rect, zmin, zmiddle);
			if (z / (5 * (cdx / 5) * mainSquareParams[1]) > 10) {
				cout << "@ ";
			}
			else {
				cout << "_ ";
			}
		}
		cout << "\n";
	}

//
//
//
//

/*if (alph != 0) {
alph = al;
}
else {

CvBox2D mbox0 = cvMinAreaRect2(markers[0]);
CvBox2D mbox1 = cvMinAreaRect2(markers[1]);
double mx0 = mbox0.center.x;
double mx1 = mbox1.center.x;
double my = (mbox0.center.y + mbox1.center.y) / 2;
double mr = (mbox0.size.width + mbox1.size.width + mbox0.size.height + mbox1.size.height) / 4;
cout << mx0 << " " << mx1 << " " << mbox0.center.y << " " << mbox1.center.y << " ";
cout << mbox0.size.width << " " << mbox1.size.width << " " << mbox0.size.height << " " << mbox1.size.height << " ";

double result = 0;
double z = 0;
double counter = 0;
double kconst = 4;
int k;
for (int i = 0; i < mx1 - mx0 - 2 * mr; i++)
{
for (int j = -3; j < 3; j++) {
k = 5 - (5 * i) / (mx1 - mx0 - 2 * mr);
k = k == 5 ? 4 : k;
if (k != kconst) {
cout << z << " ";
if (z < 50) {
result += pow(2, k);
}
kconst = k;
z = 0;
counter = 0;
}
else {
z *= counter;
int x = mx0 + mr + i;
int y = my + j;
uchar* ptr = (uchar*)(rgb->imageData + y * rgb->widthStep);
int pix = 0;
pix += (ptr[3 * x]) / 3;
pix += (ptr[3 * x] + 1) / 3;
pix += (ptr[3 * x] + 2) / 3;
if (pix < 55) {
z++;
}
//cout << r << " " << g << " " << b << " ";
counter += 1;
z /= counter;
}
}
}
cout << z << " ";
if (z < 50) {
result += pow(2, k);
}
kconst = k;
z = 0;
counter = 0;
//cout << "     " << result;
}
}
}*/
cvNamedWindow("find", 1);
cvShowImage("find", rgb);
cvNamedWindow("findnrot", 1);
cvShowImage("findnrot", rotated);

// ждём нажатия клавиши
cvWaitKey(0);

cvReleaseMemStorage(&storage);
cvReleaseMemStorage(&storageRotated);

cvReleaseImage(&src);
cvReleaseImage(&dst);
cvReleaseImage(&rgb);
cvReleaseImage(&rotated);
cvReleaseImage(&rgbMarker);
cvReleaseImage(&binI);
cvReleaseImage(&binMarker);

// удаляем окна
cvDestroyAllWindows();
}

int main() {
	IplImage *original = 0, *marker = 0;

	// исходная картинка
	char* filename = "c:/Image1.png";
	original = cvLoadImage(filename, 0);

	printf("[i] image: %s\n", filename);
	assert(original != 0);

	char* filename2 = "c:/templ.png";
	marker = cvLoadImage(filename2, 0);

	printf("[i] template: %s\n", filename2);
	assert(marker != 0);

	// суть:
	testMatch(original, marker);

	return 0;
}