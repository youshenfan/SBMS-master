// Saliency.h: interface for the Saliency class.
//
//////////////////////////////////////////////////////////////////////
//===========================================================================
//	Copyright (c) 2011 Radhakrishna Achanta [EPFL] 
//===========================================================================

#if !defined(_SALIENCY_H_INCLUDED_)
#define _SALIENCY_H_INCLUDED_

#include <vector>
#include <cfloat>
using namespace std;

class Saliency  
{
public:
	Saliency();
	virtual ~Saliency();

public:

	void GetSaliencyMap(
		const vector<UINT>&				inputimg,              //INPUT: A RGB buffer in row-major order
		const int&						width,
		const int&						height,
		vector<double>&					salmap,                //OUTPUT: Floating point buffer in row-major order
		const bool&						normalizeflag = true); //false if normalization is not needed


private:

	void RGB2LAB(
		const vector<UINT>&				ubuff,
		vector<double>&					lvec,
		vector<double>&					avec,
		vector<double>&					bvec);

	void GaussianSmooth(
		const vector<double>&			inputImg,
		const int&						width,
		const int&						height,
		const vector<double>&			kernel,
		vector<double>&					smoothImg);

	//==============================================================================
	///	Normalize
	//==============================================================================
	void Normalize(
		const vector<double>&			input,
		const int&						width,
		const int&						height,
		vector<double>&					output,
		const int&						normrange = 255)
	{
		double maxval(0);
		double minval(DBL_MAX);
		// 获得区域内最大值和最小值，获得数据范围，以进行归一化操作
		{int i(0);
		for( int y = 0; y < height; y++ )
		{
			for( int x = 0; x < width; x++ )
			{
				if( maxval < input[i] ) maxval = input[i];
				if( minval > input[i] ) minval = input[i];
				i++;
			}
		}}
		double range = maxval-minval;    // 获取最大最小范围
		if( 0 == range ) range = 1;
		
		// 进行归一化操作，将数据投射到[0,255]范围
		int i(0);
		output.clear();
		output.resize(width*height);
		for( int y = 0; y < height; y++ )
		{
			for( int x = 0; x < width; x++ )
			{
				output[i] = ((normrange*(input[i]-minval))/range);
				i++;
			}
		}
	}

};

#endif // !defined(_SALIENCY_H_INCLUDED_)
