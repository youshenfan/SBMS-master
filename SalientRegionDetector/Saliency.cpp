// Saliency.cpp: implementation of the Saliency class.
//
//////////////////////////////////////////////////////////////////////
//===========================================================================
//	Copyright (c) 2011 Radhakrishna Achanta [EPFL] 
//===========================================================================

#include "stdafx.h"
#include <cmath>
#include "Saliency.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Saliency::Saliency()
{

}

Saliency::~Saliency()
{

}


//===========================================================================
///	RGB2LAB
///
/// This is the re-written version of the sRGB to CIELAB conversion
//===========================================================================
void Saliency::RGB2LAB(
	const vector<UINT>&				ubuff,
	vector<double>&					lvec,
	vector<double>&					avec,
	vector<double>&					bvec)
{
	int sz = int(ubuff.size());
	lvec.resize(sz);
	avec.resize(sz);
	bvec.resize(sz);

	for( int j = 0; j < sz; j++ )
	{
		int sR = (ubuff[j] >> 16) & 0xFF;
		int sG = (ubuff[j] >>  8) & 0xFF;
		int sB = (ubuff[j]      ) & 0xFF;
		//------------------------
		// sRGB to XYZ conversion
		// (D65 illuminant assumption)
		//------------------------
		double R = sR/255.0;
		double G = sG/255.0;
		double B = sB/255.0;

		double r, g, b;

		if(R <= 0.04045)	r = R/12.92;
		else				r = pow((R+0.055)/1.055,2.4);
		if(G <= 0.04045)	g = G/12.92;
		else				g = pow((G+0.055)/1.055,2.4);
		if(B <= 0.04045)	b = B/12.92;
		else				b = pow((B+0.055)/1.055,2.4);

		double X = r*0.4124564 + g*0.3575761 + b*0.1804375;             // RGB转化到XYZ颜色空间
		double Y = r*0.2126729 + g*0.7151522 + b*0.0721750;
		double Z = r*0.0193339 + g*0.1191920 + b*0.9503041;
		//------------------------
		// XYZ to LAB conversion
		// 转化算法参见： https://blog.csdn.net/lz0499/article/details/77345166
		//------------------------
		double epsilon = 0.008856;	//actual CIE standard
		double kappa   = 903.3;		//actual CIE standard

		double Xr = 0.950456;	//reference white
		double Yr = 1.0;		//reference white
		double Zr = 1.088754;	//reference white

		double xr = X/Xr;
		double yr = Y/Yr;
		double zr = Z/Zr;

		double fx, fy, fz;
		if(xr > epsilon)	fx = pow(xr, 1.0/3.0);
		else				fx = (kappa*xr + 16.0)/116.0;
		if(yr > epsilon)	fy = pow(yr, 1.0/3.0);
		else				fy = (kappa*yr + 16.0)/116.0;
		if(zr > epsilon)	fz = pow(zr, 1.0/3.0);
		else				fz = (kappa*zr + 16.0)/116.0;

		lvec[j] = 116.0*fy-16.0;
		avec[j] = 500.0*(fx-fy);
		bvec[j] = 200.0*(fy-fz);
	}
}

//==============================================================================
///	GaussianSmooth
///
///	Blur an image with a separable binomial kernel passed in.
//==============================================================================
void Saliency::GaussianSmooth(
	const vector<double>&			inputImg,
	const int&						width,
	const int&						height,
	const vector<double>&			kernel,
	vector<double>&					smoothImg)
{
	int center = int(kernel.size())/2;

	int sz = width*height;
	smoothImg.clear();
	smoothImg.resize(sz);
	vector<double> tempim(sz);
	int rows = height;
	int cols = width;
   //--------------------------------------------------------------------------
   // Blur in the x direction.
   //---------------------------------------------------------------------------
	{int index(0);
	for( int r = 0; r < rows; r++ )
	{
		for( int c = 0; c < cols; c++ )
		{
			double kernelsum(0);
			double sum(0);
			for( int cc = (-center); cc <= center; cc++ )
			{
				if(((c+cc) >= 0) && ((c+cc) < cols))
				{
					sum += inputImg[r*cols+(c+cc)] * kernel[center+cc];
					kernelsum += kernel[center+cc];
				}
			}
			tempim[index] = sum/kernelsum;
			index++;
		}
	}}

	//--------------------------------------------------------------------------
	// Blur in the y direction.
	//---------------------------------------------------------------------------
	{int index = 0;
	for( int r = 0; r < rows; r++ )
	{
		for( int c = 0; c < cols; c++ )
		{
			double kernelsum(0);
			double sum(0);
			for( int rr = (-center); rr <= center; rr++ )
			{
				if(((r+rr) >= 0) && ((r+rr) < rows))
				{
				   sum += tempim[(r+rr)*cols+c] * kernel[center+rr];
				   kernelsum += kernel[center+rr];
				}
			}
			smoothImg[index] = sum/kernelsum;
			index++;
		}
	}}
}

//===========================================================================
///	GetSaliencyMap
/// 计算获得显著性图
/// Outputs a saliency map with a value assigned per pixel. The values are
/// normalized in the interval [0,255] if normflag is set true (default value).
//===========================================================================
void Saliency::GetSaliencyMap(
	const vector<UINT>&				inputimg,
	const int&						width,
	const int&						height,
	vector<double>&					salmap,
	const bool&						normflag) 
{
	int sz = width*height;
	salmap.clear();
	salmap.resize(sz);

	vector<double> lvec(0), avec(0), bvec(0);
	RGB2LAB(inputimg, lvec, avec, bvec);

	//--------------------------
	// Obtain Lab average values
	//--------------------------
	double avgl(0), avga(0), avgb(0);
	{for( int i = 0; i < sz; i++ )
	{
		avgl += lvec[i];
		avga += avec[i];
		avgb += bvec[i];
	}}
	avgl /= sz;          // 获得L的平均值
	avga /= sz;          // 获得a的平均值
	avgb /= sz;          // 获得b的平均值

	vector<double> slvec(0), savec(0), sbvec(0);

	//----------------------------------------------------
	// The kernel can be [1 2 1] or [1 4 6 4 1] as needed.
	// The code below show usage of [1 2 1] kernel.
	// 核是用来进行高斯光滑的
	//----------------------------------------------------
	vector<double> kernel(0);
	kernel.push_back(1.0);
	kernel.push_back(2.0);
	kernel.push_back(1.0);
	
	// 使用[1 4 6 4 1]核的方法【一般会使显著得表现得更白一些，但差别不大】
	// kernel.push_back(1.0);
	// kernel.push_back(4.0);
	// kernel.push_back(6.0);
	// kernel.push_back(4.0);
	// kernel.push_back(1.0);


	GaussianSmooth(lvec, width, height, kernel, slvec);    // 获得高斯平滑处理后的L信息slvec
	GaussianSmooth(avec, width, height, kernel, savec);
	GaussianSmooth(bvec, width, height, kernel, sbvec);


	// 源程序的平方和
	//{for( int i = 0; i < sz; i++ )                       // 统计平方和作为显著度值
	//{
	//	salmap[i] = (slvec[i]-avgl)*(slvec[i]-avgl) +
	//				(savec[i]-avga)*(savec[i]-avga) +
	//				(sbvec[i]-avgb)*(sbvec[i]-avgb);
	//}}


	// FWQ修改后的显著度值计算程序
	// 发现规律：确实可以通过调整系数，调节绿地变为不显著值，但经过均值漂移，是大块连起来的地方作为显著性区域显示的。
	{for (int i = 0; i < sz; i++)                       // 统计平方和作为显著度值
	{
		salmap[i] = (slvec[i] - avgl)*(slvec[i] - avgl) +
			        (savec[i] - avga)*(savec[i] - avga) +
			        0.001* (sbvec[i] - avgb)*(sbvec[i] - avgb);
	}}
	/// FWQ修改后的显著度值计算程序


	if( true == normflag )
	{
		vector<double> normalized(0);
		Normalize(salmap, width, height, normalized);
		swap(salmap, normalized);
	}



	// 测试绘图Lab空间图
	// vector<double> testImg;
	// testImg.resize(sz);
	// {for (int i = 0; i < sz; i++)                       // 统计平方和作为显著度值
	// {
	//	testImg[i] = bvec[i];
	// }}
	// salmap = testImg;



}