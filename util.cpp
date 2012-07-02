#include "util.h"
#include <iostream>
using namespace cv;

void render_segment(Mat& src, Mat& dst)
{
	if (src.type()!=CV_8UC1)
	{
		std::cout<<"segment image must be CV_8UC1."<<std::endl;
		return;
	}
	Scalar colors[]={Scalar(0,255,255),Scalar(255,0,0),Scalar(0,255,0),Scalar(0,0,255),Scalar(255,0,255)};
	vector<Segment> segStack;

	IplImage src_=src;//no copy is done
	IplImage* segMap=cvCreateImage(cvGetSize(&src_),IPL_DEPTH_32F,1);

	segment(&src_,&segStack,segMap);
	dst.setTo(Scalar(0,0,0));
	RgbImage img_dst(dst);
	BwImageFloat_ img_S(segMap);
	for (int i=0;i<dst.rows;i++)
	{
		for (int j=0;j<dst.cols;j++)
		{
			int id=img_S[i][j];
			if (id>0)
			{
				int color_idx=id%5;
				img_dst[i][j].b=colors[color_idx].val[0];
				img_dst[i][j].g=colors[color_idx].val[1];
				img_dst[i][j].r=colors[color_idx].val[2];
			}

		}
	}
	cvReleaseImage(&segMap);
}

inline bool isIdentical(CvPoint a,CvPoint b,BwImage_ img)
{
	double dist=fabs(double(img[a.y][a.x])-img[b.y][b.x]);
	if (dist>127) return false;
	return true;
}
void segment(IplImage* src,vector<Segment>* segStack,IplImage* segMap,int areaLB, int areaHB)
{
	CvMemStorage* storage=cvCreateMemStorage(0);
	int width=src->width;
	int height=src->height;
	CvPoint startP=cvPoint(0,0);
	CvPoint endP=cvPoint(width-1,height-1);

	cvZero(segMap);
	BwImageFloat_ imgS(segMap);
	BwImage_ imgMa(src);
	for (int i=0;i<height;i++)
	{
		for (int j=0;j<width;j++)
		{
			if (imgMa[i][j]==0)
				imgS[i][j]=-1;//label the masked pixel
			else
				imgS[i][j]=0;//0 means the pixel has not been visited
		}
	}

	//	int i=startP.y;
	//	int j=startP.x;
	//	double s;
	int skinCount=1;

	CvSeq* pointStack=cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),storage);

	for(int i = startP.y;i <= endP.y;i++)
	{
		for(int j = startP.x;j <= endP.x;j++)
		{

			if(imgS[i][j]!=0) continue;

			Segment skin;
			skin.ID=skinCount;


			CvPoint p=cvPoint(j,i);
			cvSeqPush(pointStack, &p);
			imgS[p.y][p.x]=skinCount;
			int skin_area=0;

			int left=width;
			int up=height;
			int right=0;
			int down=0;

			while(pointStack->total>0)
			{
				CvPoint p_pop;
				cvSeqPop(pointStack, &p_pop);

				skin_area++;

				//				cvmSet(segMap,p_pop.y,p_pop.x,skinCount);

				if (left>p_pop.x)	left=p_pop.x;
				if (up>p_pop.y)		up=p_pop.y;
				if (right<p_pop.x)	right=p_pop.x;
				if (down<p_pop.y)	down=p_pop.y;

				if(p_pop.x >= startP.x && p_pop.x <= endP.x && p_pop.y >= startP.y && p_pop.y <= endP.y)
				{
					if(p_pop.x>startP.x){
						if( imgS[p_pop.y][p_pop.x-1]==0 && 
							isIdentical(cvPoint(p_pop.x-1,p_pop.y),p_pop,imgMa)
							)
						{
							CvPoint pp=cvPoint(p_pop.x-1,p_pop.y);
							cvSeqPush(pointStack, &pp);
							imgS[pp.y][pp.x]=skinCount;
						}
					}

					if(p_pop.x<endP.x){
						if( imgS[p_pop.y][p_pop.x+1]==0 && 
							isIdentical(cvPoint(p_pop.x+1,p_pop.y),p_pop,imgMa)
							)
						{
							CvPoint pp=cvPoint(p_pop.x+1,p_pop.y);
							cvSeqPush(pointStack, &pp);
							imgS[pp.y][pp.x]=skinCount;
						}
					}

					if(p_pop.y>startP.y){
						if( imgS[p_pop.y-1][p_pop.x]==0 && 
							isIdentical(cvPoint(p_pop.x,p_pop.y-1),p_pop,imgMa)
							)
						{
							CvPoint pp=cvPoint(p_pop.x,p_pop.y-1);
							cvSeqPush(pointStack, &pp);
							imgS[pp.y][pp.x]=skinCount;
						}
					}

					if(p_pop.y<endP.y){
						if( imgS[p_pop.y+1][p_pop.x]==0 && 
							isIdentical(cvPoint(p_pop.x,p_pop.y+1),p_pop,imgMa)
							)
						{
							CvPoint pp=cvPoint(p_pop.x,p_pop.y+1);
							cvSeqPush(pointStack, &pp);
							imgS[pp.y][pp.x]=skinCount;
						}
					}
				}

			}
			if (skin_area>areaLB && (skin_area<areaHB || areaHB<0)){
				skin.ID = skinCount;
				skin.area=skin_area;
				skin.up_left=cvPoint(left,up);
				skin.low_right=cvPoint(right,down);
				segStack->push_back(skin);
			}
			skinCount++;
		}
	}
	cvReleaseMemStorage(&storage);
}
void detectSkin(Mat& src,Mat& dst)
{
	dst.setTo(Scalar(0));
	RgbImage img_src(src);
	BwImage img_dst(dst);
	for (int i=0; i<src.rows; ++i)
	{
		for (int j=0; j<src.cols; ++j)
		{
			int b=img_src[i][j].b;
			int g=img_src[i][j].g;
			int r=img_src[i][j].r;
			if (r>95 && g>40 && b>20 && r>g && r>b && abs(r-b)>15 && max(r,max(g,b))-min(r,min(g,b))>55)
			{
				img_dst[i][j]=225;
			}
		}
	}
}
void threshold_ex(Mat& src,Mat& dst,double c,int mode)
{
	if (mode==THRESHOLD_ADAPTIVE)
	{
		Mat mean;
		medianBlur(src,mean,5);
		BwImage img_mean(mean);
		BwImage img_dst(dst);
		BwImage img_src(src);
		for (int i=0;i<dst.rows;++i)
		{
			for (int j=0;j<dst.cols;++j)
			{
				if (img_src[i][j]<img_mean[i][j]-c)
				{
					img_dst[i][j]=0;
				}
				else
					img_dst[i][j]=255;
			}
		}
	}
	else if (mode==(THRESHOLD_ADAPTIVE|THRESHOLD_INVERSE))
	{
		Mat mean;
		medianBlur(src,mean,9);
		BwImage img_mean(mean);
		BwImage img_dst(dst);
		BwImage img_src(src);
		for (int i=0;i<dst.rows;++i)
		{
			for (int j=0;j<dst.cols;++j)
			{
				if (img_src[i][j]>=img_mean[i][j]-c)
				{
					img_dst[i][j]=0;
				}
				else
					img_dst[i][j]=255;
			}
		}
	}
	else if (mode==THRESHOLD_ABSOLUTE)
	{
		BwImage img_dst(dst);
		BwImage img_src(src);
		for (int i=0;i<dst.rows;++i)
		{
			for (int j=0;j<dst.cols;++j)
			{
				if (img_src[i][j]<c)
				{
					img_dst[i][j]=0;
				}
				else
					img_dst[i][j]=255;
			}
		}
	}
	else if (mode==(THRESHOLD_ABSOLUTE|THRESHOLD_INVERSE))
	{
		BwImage img_dst(dst);
		BwImage img_src(src);
		for (int i=0;i<dst.rows;++i)
		{
			for (int j=0;j<dst.cols;++j)
			{
				if (img_src[i][j]>=c)
				{
					img_dst[i][j]=0;
				}
				else
					img_dst[i][j]=255;
			}
		}
	}	
}