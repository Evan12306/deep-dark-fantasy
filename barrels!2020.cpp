/*
作者：刘昕阳
最后修改时间：2020.12.27
功能：分离出近乎纯色的桶，进行识别定位
*/

#include <opencv2/opencv.hpp>
#include<opencv2/core/core.hpp>
//#include <opencv2/xfeatures2d/nonfree.hpp>

using namespace cv;
using namespace std;
//using namespace xfeatures2d;

/*图像的命名*/
//输入图像  
Mat img;
//灰度值归一化 
Mat bgr;
//HSV图像  
Mat hsv;
//输出图像  
Mat dst;

/*滑动条程序的阈值*/

//色相  H
int hmin_Max = 360;
int hmax_Max = 360;
//饱和度  S
int smin_Max = 255;
int smax_Max = 255;
//亮度  V
int vmin_Max = 255;
int vmax_Max = 255;

//引用图像处理函数
void MaskCreated_Blue(Mat& img, Mat& mask_Blue0);
void MaskCreated_Red(Mat& img, Mat& mask_Red0);

int main()
{
 ////////////在这里修改图片，格式为X_Color.png, X的取值范围为1——11/////////////
	Mat srcImage = imread("11_Color.png", 1);

	namedWindow("原图");
	imshow("原图", srcImage);
	//输出图像分配内存  

	Mat mask_Blue0 = Mat::zeros(srcImage.size(), CV_32FC3);
	Mat mask_Red0 = Mat::zeros(srcImage.size(), CV_32FC3);

	if (!srcImage.data || srcImage.channels() != 3)
	{
		printf("喵");
	}

	/* 图像处理 */
	MaskCreated_Blue(srcImage, mask_Blue0);
	MaskCreated_Red(srcImage, mask_Red0);

	Mat blue = Mat::zeros(srcImage.size(), CV_8UC3);
	Mat red = Mat::zeros(srcImage.size(), CV_8UC3);

	/* 提取轮廓_蓝 */
	mask_Blue0.convertTo(blue, CV_8UC3, 255);

	vector<vector<Point>> contoursBlue;
	vector<Vec4i> hierarchyBlue;
	findContours(blue, contoursBlue, hierarchyBlue, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//绘制最小边界矩阵
	Point2f pts[4];

	for (int i = 0; i < contoursBlue.size(); i++)
	{   
		//将蓝桶用蓝色圈出
		Scalar colors = Scalar(255, 0, 0);

		RotatedRect rects = minAreaRect(contoursBlue[i]);
		rects.points(pts);//确定旋转矩阵的四个顶点  
		for (int j = 0; j < 4; j++)
		{
			line(srcImage, pts[j], pts[(j + 1) % 4], colors, 2);
		}
		//绘制中心点
		Point2f cptBlue = rects.center;
		circle(srcImage, cptBlue, 2, Scalar(255, 0, 0), 2, 8, 0);
	}

	/* 提取轮廓_红 */
	mask_Red0.convertTo(red, CV_8UC3, 255);

	vector<vector<Point>> contoursRed;
	vector<Vec4i> hierarchyRed;
	findContours(red, contoursRed, hierarchyRed, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//绘制最小边界矩阵
	Point2f pts1[4];

	for (int m = 0; m < contoursRed.size(); m++)
	{
		//将红桶用红色圈出
		Scalar colors0 = Scalar(0, 0, 255);

		RotatedRect rects = minAreaRect(contoursRed[m]);
		rects.points(pts1);//确定旋转矩阵的四个顶点
		for (int n = 0; n < 4; n++)
		{
			line(srcImage, pts1[n], pts1[(n + 1) % 4], colors0, 2);
		}
		//绘制中心点
		Point2f cptRed = rects.center;
		circle(srcImage, cptRed, 2, Scalar(0, 0, 255), 2, 8, 0);
	}

	//最终成品展示
	imshow("识别结果", srcImage);

	//存储成果
	imwrite("一次成功的识别！.png", srcImage);

	waitKey(0);
	return 0;
}

void MaskCreated_Blue(Mat& img, Mat& mask_Blue0)
{
	//H
	int hminBlue = 210;
	int hmaxBlue = 230;
	//S
	int sminBlue = 160;
	int smaxBlue = 230;
	//V
	int vminBlue = 55;
	int vmaxBlue = 135;

	//灰度值归一化  
	Mat bgr;
	//HSV图像  
	Mat hsv;

	Mat mask_Blue = Mat::zeros(img.size(), CV_32FC3);

	Mat element0 = getStructuringElement(MORPH_RECT, Size(5, 5));
	//闭操作 (连接一些连通域)
	morphologyEx(img, img, MORPH_CLOSE, element0);
	imshow("img", img);

	//彩色图像的灰度值归一化  
	img.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);
	//颜色空间转换  
	cvtColor(bgr, hsv, COLOR_BGR2HSV);

	inRange(hsv, Scalar(hminBlue, sminBlue / float(smin_Max), vminBlue / float(vmin_Max)),
		    Scalar(hmaxBlue, smaxBlue / float(smax_Max), vmaxBlue / float(vmax_Max)), mask_Blue0);
	//只保留  
	for (int r = 0; r < bgr.rows; r++)
	{
		for (int c = 0; c < bgr.cols; c++)
		{
			if (mask_Blue0.at<uchar>(r, c) == 255)//白色部分保留原色
			{
				mask_Blue.at<Vec3f>(r, c) = bgr.at<Vec3f>(r, c);
			}
		}
	}

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
	//开操作（去除一些噪点）
	morphologyEx(mask_Blue0, mask_Blue0, MORPH_OPEN, element);

	Mat element1 = getStructuringElement(MORPH_RECT, Size(8, 8));//在图片中队旗和架子上的蓝色物体是干扰项
	//闭操作 (连接一些连通域，这里指的是将残缺的桶布满)
	morphologyEx(mask_Blue0, mask_Blue0, MORPH_CLOSE, element1);

	Mat element2 = getStructuringElement(MORPH_RECT, Size(6, 6));
	//再次开操作（闭操作后桶的部分被填充，其他部分会被孤立，易于清除）
	morphologyEx(mask_Blue0, mask_Blue0, MORPH_OPEN, element2);

	imshow("mask_Blue", mask_Blue);

}

void MaskCreated_Red(Mat& img, Mat& mask_Red0)
{
	//H
	int hminRed = 0;
	int hmaxRed = 19;
	//S
	int sminRed = 140;
	int smaxRed = 210;
	//V
	int vminRed = 105;
	int vmaxRed = 160;
	//灰度值归一化  
	Mat bgr;
	//HSV图像  
	Mat hsv;

	Mat mask_Red = Mat::zeros(img.size(), CV_32FC3);
	Mat element0 = getStructuringElement(MORPH_RECT, Size(5, 5));
	//闭操作 (连接一些连通域)
	morphologyEx(img, img, MORPH_CLOSE, element0);

	//彩色图像的灰度值归一化  
	img.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);
	//颜色空间转换  
	cvtColor(bgr, hsv, COLOR_BGR2HSV);

	//掩码  
	inRange(hsv, Scalar(hminRed, sminRed / float(smin_Max), vminRed / float(vmin_Max)),
		    Scalar(hmaxRed, smaxRed / float(smax_Max), vmaxRed / float(vmax_Max)), mask_Red0);
	//只保留  
	for (int r = 0; r < bgr.rows; r++)
	{
		for (int c = 0; c < bgr.cols; c++)
		{
			if (mask_Red0.at<uchar>(r, c) == 255)
			{
				mask_Red.at<Vec3f>(r, c) = bgr.at<Vec3f>(r, c);
			}
		}
	}

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
	//开操作（去除一些噪点）
	morphologyEx(mask_Red0, mask_Red0, MORPH_OPEN, element);

	Mat element1 = getStructuringElement(MORPH_RECT, Size(10, 10));
	//在图片中屏幕上的字和棕色地板是干扰项，这部分在开操作后被分离的很彻底，可以加大力度

	//闭操作 (连接一些连通域)
	morphologyEx(mask_Red0, mask_Red0, MORPH_CLOSE, element1);

	Mat element2 = getStructuringElement(MORPH_RECT, Size(6, 6));
	//再次开操作（去除一些噪点）
	morphologyEx(mask_Red0, mask_Red0, MORPH_OPEN, element2);

	imshow("mask_Red", mask_Red);
}
