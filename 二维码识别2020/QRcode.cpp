/********************************************************************
作者：刘昕阳 
最后一次修改时间：2021.2.22
功能：定位二维码
NOTICE：筛选条件与最小外接矩形相关，在角度较苛刻的条件下难以判定成功 _(:3_\)_
*********************************************************************/
#include<iostream>
#include<opencv2/opencv.hpp>
#include<opencv2/core/core.hpp>

using namespace cv;
using namespace std;

int main(void)
{
    //~1.图像导入~//
    Mat img = imread("1.jpg");
    //resize(img, img, Size(640, 480));//此处的尺寸以回字形贴近正方形为佳
    Mat gray;
    Mat src;
    Mat paint = Mat::zeros(img.size(),CV_8UC3);

    imshow("原图", img);

    //~2.图像处理~//

    //灰度转换
    cvtColor(img, gray, COLOR_BGR2GRAY);
    //模糊处理
    //blur(gray, src, Size(3, 3));
    //直方图均衡化
    //equalizeHist(gray, src);

    //二值化处理
    adaptiveThreshold(gray, src, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 29, 5);
    //自适应二值化在背景复杂的情况下使用，在二维码明显的情况下会在回的黑色方框形成洞，形成第三层子轮廓

    //threshold(src, src, 40, 255, THRESH_BINARY);
    //二值化可以保护回字形内部纯净，缺点是灵活性较差

    imshow("图像处理后", src);


    //~3.寻找有两个子轮廓的父轮廓~//

    vector<vector<Point>> contours, contoursLocked;
    vector<Vec4i> hierarchy;

    int lockedFlag = 0;//该轮廓有父轮廓的标志
    int parentIdx = -1;

    findContours(src, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++)
    {
        if (hierarchy[i][2] != -1 && lockedFlag == 0)//存在子轮廓且标记为0
        {
            parentIdx = i;
            lockedFlag++;
            
        }
        else if (hierarchy[i][2] != -1)//存在子轮廓
        {
            lockedFlag++;
        }
        else if (hierarchy[i][2] == -1)//不存在子轮廓
        {  
            lockedFlag = 0;
            parentIdx = -1;
        }
        if (lockedFlag >= 2)//在回形过于明显的情况下用自适应二值化可以在回内部形成一个洞，制造第三层子轮廓，除杂的效果会更好
        {
            contoursLocked.push_back(contours[parentIdx]);

            lockedFlag = 0;
            parentIdx = -1;
        }
    }

    //~4.轮廓二次筛选~//

    printf("以下是轮廓面积与外接矩形面积：\n");

    vector<vector<Point>> realContours;

    for (size_t i = 0; i < contoursLocked.size(); i++)
    {
        if (i >= contoursLocked.size() || i < 0) //检测下标是否越界
        {
            cout << "vector下标越界" << endl;
            break; 
        }

        //计算轮廓面积
        float area = contourArea(contoursLocked[i]);

        //求外接最小矩形
        RotatedRect rects = minAreaRect(contoursLocked[i]);

        Point2f vertex[4];//用于存放最小矩形的四个顶点
        rects.points(vertex);//返回矩形的四个顶点给vertex

        //绘制最小面积包围矩形
        vector<Point>min_rectangle;
        for (int m = 0; m < 4; m++)
        {
            line(gray, vertex[m], vertex[(m + 1) % 4], Scalar(0, 255, 0), 1, 8);
            min_rectangle.push_back(vertex[m]);
        }
        //计算矩形轮廓面积
        float rectArea = contourArea(min_rectangle);
        //展示环节
        printf("%f %f\n", area, rectArea);

        //筛选条件是两者面积比值大于0.8，排除过大过小要求
        if (area / rectArea > 0.8 && area < 10000 && area >150)
        {
            realContours.push_back(contoursLocked[i]);
            //drawContours(img, contoursLocked, i, Scalar(0, 255, 255), 2, 8);
            drawContours(paint, contoursLocked, i, Scalar(0, 255, 255), 2, 8);
            printf("此为定位轮廓\n");
        }    
    }
  
    if (realContours.size() != 3)//如果筛选有误
    {
        printf("收到的定位轮廓不是3个，回去改改参数吧");
    }

    //~5.圈出二维码~//

    //分别求出三个轮廓的最小外接矩形
    RotatedRect box = minAreaRect(realContours[0]);
    RotatedRect box1 = minAreaRect(realContours[1]);
    RotatedRect box2 = minAreaRect(realContours[2]);

    Point2f point[3];
    //找到三个轮廓的中心点
    for (int i = 0; i < 3; i++)
    {
        RotatedRect box = minAreaRect(realContours[i]);
        point[i] = box.center;
    }

    //连接中心点，使其成为一个整体
    line(paint, point[0], point[1], Scalar(0, 0, 255), 20, 8);
    line(paint, point[1], point[2], Scalar(0, 0, 255), 20, 8);
    line(paint, point[0], point[2], Scalar(0, 0, 255), 20, 8);
        
    //展示连接效果
    imshow("提取中...", paint);

    //重新提取轮廓
    vector<vector<Point> > contours_all;
    vector<Vec4i> hierarchy_all;

    cvtColor(paint, paint, COLOR_BGR2GRAY);
    threshold(paint, paint, 45, 255, THRESH_BINARY);

    findContours(paint, contours_all, hierarchy_all, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    Point2f fourPoint2f[4];
    //求最小包围矩形
    RotatedRect rectPoint = minAreaRect(contours_all[0]);

    //将rectPoint变量中存储的坐标值放到 fourPoint的数组中
    rectPoint.points(fourPoint2f);

    for (int i = 0; i < 4; i++)
    {
        line(img, fourPoint2f[i], fourPoint2f[(i + 1) % 4]
            , Scalar(255, 0, 0), 3, 8);
    }

  
    imshow("成品", img);
    waitKey(0);
}
