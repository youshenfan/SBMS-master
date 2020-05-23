
// SalientRegionDetectorDlg.cpp : implementation file
//
//===========================================================================
// This code implements the saliency detection and segmentation method described in:
// 该程序完成了如下文献描述的显著性检测和分割方法：
// R. Achanta, S. Hemami, F. Estrada and S. S. strunk, Frequency-tuned Salient Region Detection,
// IEEE International Conference on Computer Vision and Pattern Recognition (CVPR), 2009
//===========================================================================
//	Copyright (c) 2010 Radhakrishna Achanta [EPFL].
//===========================================================================
// Email: firstname.lastname@epfl.ch
//////////////////////////////////////////////////////////////////////
//===========================================================================
//	  This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//===========================================================================


#include "stdafx.h"
#include "SalientRegionDetector.h"
#include "SalientRegionDetectorDlg.h"
#include "PictureHandler.h"
#include "Saliency.h"
#include "MeanShiftCode/msImageProcessor.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;
using namespace cv;

// CSalientRegionDetectorDlg dialog




CSalientRegionDetectorDlg::CSalientRegionDetectorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSalientRegionDetectorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSalientRegionDetectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSalientRegionDetectorDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_DETECTSALIENCY, &CSalientRegionDetectorDlg::OnBnClickedButtonDetectSaliency)
	ON_BN_CLICKED(IDC_CHECK_SEGMENTATIONFLAG, &CSalientRegionDetectorDlg::OnBnClickedCheckSegmentationFlag)
END_MESSAGE_MAP()


// CSalientRegionDetectorDlg message handlers

BOOL CSalientRegionDetectorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_segmentationflag = false;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSalientRegionDetectorDlg::OnBnClickedCheckSegmentationFlag()
{
	if(false == m_segmentationflag) m_segmentationflag = true;
	else m_segmentationflag = false;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSalientRegionDetectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSalientRegionDetectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//=================================================================================
///	GetPictures  获取图片
///
///	This function collects all the pictures the user chooses into a vector.
/// 该函数将用户选择的所有图片整理到一个vector中
//=================================================================================
void CSalientRegionDetectorDlg::GetPictures(vector<string>& picvec)
{
	CFileDialog cfd(TRUE,NULL,NULL,OFN_OVERWRITEPROMPT,L"*.*|*.*|",NULL);
	cfd.m_ofn.Flags |= OFN_ALLOWMULTISELECT;

	//cfd.PostMessage(WM_COMMAND, 40964, NULL);
	
	CString strFileNames;
	cfd.m_ofn.lpstrFile = strFileNames.GetBuffer(2048);
	cfd.m_ofn.nMaxFile = 2048;

	BOOL bResult = cfd.DoModal() == IDOK ? TRUE : FALSE;
	strFileNames.ReleaseBuffer();

	//if(cfd.DoModal() == IDOK)
	if( bResult )
	{
		POSITION pos = cfd.GetStartPosition();
		while (pos) 
		{
			CString imgFile = cfd.GetNextPathName(pos);			
			PictureHandler ph;
			string name = ph.Wide2Narrow(imgFile.GetString());
			picvec.push_back(name);
		}
	}
	else return;
}

//===========================================================================
///	BrowseForFolder
//===========================================================================
bool CSalientRegionDetectorDlg::BrowseForFolder(string& folderpath)
{
	IMalloc* pMalloc = 0;
	if(::SHGetMalloc(&pMalloc) != NOERROR)
	return false;

	BROWSEINFO bi;
	memset(&bi, 0, sizeof(bi));

	bi.hwndOwner = m_hWnd;
	bi.lpszTitle = L"Please select a folder and press 'OK'.";

	LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);
	if(pIDL == NULL)
	return false;

	TCHAR buffer[_MAX_PATH];
	if(::SHGetPathFromIDList(pIDL, buffer) == 0)
	return false;
	PictureHandler pichand;
	folderpath = pichand.Wide2Narrow(buffer);
	folderpath.append("\\");
	return true;
}

//===========================================================================
///	DoMeanShiftSegmentation   进行均值漂移分割
//===========================================================================
void CSalientRegionDetectorDlg::DoMeanShiftSegmentation(
	const vector<UINT>&						inputImg,
	const int&								width,
	const int&								height,
	vector<UINT>&							segimg,
	const int&								sigmaS,
	const float&							sigmaR,
	const int&								minRegion,
	vector<int>&							labels,
	int&									numlabels)
{
	int sz = width*height;
	BYTE* bytebuff = new BYTE[sz*3];
	{int i(0);
	for( int p = 0; p < sz; p++ )
	{
		bytebuff[i+0] = inputImg[p] >> 16 & 0xff;       // 右移2字节截取
		bytebuff[i+1] = inputImg[p] >>  8 & 0xff;       // 右移1字节截取
		bytebuff[i+2] = inputImg[p]       & 0xff;       // 直接     截取
		i += 3;                                         // 3字节3字节移位
	}}
	msImageProcessor mss;
	mss.DefineImage(bytebuff, COLOR, height, width);		
	mss.Segment(sigmaS, sigmaR, minRegion, HIGH_SPEEDUP);
	mss.GetResults(bytebuff);

	int* p_labels = new int[sz];
	numlabels = mss.GetLabels(p_labels);
	labels.resize(sz);
	for( int n = 0; n < sz; n++ ) labels[n] = p_labels[n];
	if(p_labels) delete [] p_labels;

	segimg.resize(sz);
	int bsz = sz*3;
	{int i(0);
	for( int p = 0; p < bsz; p += 3 )
	{
		segimg[i] = bytebuff[p] << 16 | bytebuff[p+1] << 8 | bytebuff[p+2];
		i++;
	}}
	if(bytebuff) delete [] bytebuff;
}

//=================================================================================
/// DrawContoursAroundSegments
//=================================================================================
void CSalientRegionDetectorDlg::DrawContoursAroundSegments(
	vector<UINT>&							segmentedImage,
	const int&								width,
	const int&								height,
	const UINT&								color)
{
	// Pixel offsets around the centre pixels starting from left, going clockwise
	const int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
	const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};

	int sz = segmentedImage.size();
	vector<bool> istaken(sz, false);
	int mainindex(0);
	for( int j = 0; j < height; j++ )
	{
		for( int k = 0; k < width; k++ )
		{
			int np(0);
			for( int i = 0; i < 8; i++ )
			{
				int x = k + dx8[i];
				int y = j + dy8[i];

				if( (x >= 0 && x < width) && (y >= 0 && y < height) )
				{
					int index = y*width + x;
					if( false == istaken[index] )
					{
						if( (int)segmentedImage[mainindex] != (int)segmentedImage[index] ) np++;
					}
				}
			}
			if( np > 2 )//1 for thicker lines and 2 for thinner lines
			{
				segmentedImage[j*width + k] = color;
				istaken[mainindex] = true;
			}
			mainindex++;
		}
	}
}


//=================================================================================
// ChooseSalientPixelsToShow
//=================================================================================
void CSalientRegionDetectorDlg::ChooseSalientPixelsToShow(
	const vector<double>&					salmap,
	const int&								width,
	const int&								height,
	const vector<int>&						labels,
	const int&								numlabels,
	vector<bool>&							choose)
{
	int sz = width*height;
	//----------------------------------
	// Find average saliency per segment
	//----------------------------------
	vector<double> salperseg(numlabels,0);
	vector<int> segsz(numlabels,0);
	vector<bool> touchborders(numlabels, false);
	{int i(0);
	for( int j = 0; j < height; j++ )
	{
		for( int k = 0; k < width; k++ )
		{
			salperseg[labels[i]] += salmap[i];
			segsz[labels[i]]++;
			
			if(false == touchborders[labels[i]] && (j == height-1 || j == 0 || k == width-1 || k == 0) )
			{
				touchborders[labels[i]] = true;
			}
			i++;
		}
	}}

	double avgimgsal(0);
	{for( int n = 0; n < numlabels; n++ )
	{
		if(true == touchborders[n])
		{
			salperseg[n] = 0;
		}
		else
		{
			avgimgsal += salperseg[n];
			salperseg[n] /= segsz[n];
		}
	}}

	//--------------------------------------
	// Compute average saliency of the image
	//--------------------------------------
	avgimgsal /= sz;


	//----------------------------------------------------------------------------
	// Choose segments that have average saliency twice the average image saliency
	//----------------------------------------------------------------------------
	vector<bool> segtochoose(numlabels, false);
	{for( int n = 0; n < numlabels; n++ )
	{
		if( salperseg[n] > (avgimgsal+avgimgsal) ) segtochoose[n] = true;
	}}

	choose.resize(sz, false);
	bool atleastonesegmentchosent(false);
	{for( int s = 0; s < sz; s++ )
	{
		//if( salperseg[labels[s]] > (avgsal+avgsal) )
		//if(true == segtochoose[labels[s]])
		{
			choose[s] = segtochoose[labels[s]];
			atleastonesegmentchosent = choose[s];
		}
	}}

	//----------------------------------------------------------------------------
	// If not a single segment has been chosen, then take the brightest one available
	//----------------------------------------------------------------------------
	if( false == atleastonesegmentchosent )
	{
		int maxsalindex(-1);
		double maxsal(DBL_MIN);
		for( int n = 0; n < numlabels; n++ )
		{
			if( maxsal < salperseg[n] )
			{
				maxsal = salperseg[n];
				maxsalindex = n;
			}
		}
		for( int s = 0; s < sz; s++ )
		{
			if(maxsalindex == labels[s]) choose[s] = true;
		}
	}
}

//===========================================================================
///	DoMeanShiftSegmentationBasedProcessing
/// 进行基于均值漂移分割的处理
///
///	Do the segmentation of salient region based on K-Means segmentation
/// 进行基于K-means分割的显著性区域分割
//===========================================================================
void CSalientRegionDetectorDlg::DoMeanShiftSegmentationBasedProcessing(
	const vector<UINT>&						inputImg,
	const int&								width,
	const int&								height,
	const string&							filename,
	const vector<double>&					salmap,
	const int&								sigmaS,
	const float&							sigmaR,
	const int&								minRegion,
	vector<UINT>&							segimg,
	vector<UINT>&							segobj,
	vector<vector<UINT>>&                    imgclustering)
{
	int sz = width*height;
	//--------------------------------------------------------------------
	// Segment the image using mean-shift algo. Segmented image in segimg.
	//--------------------------------------------------------------------
	vector<int> labels(0);
	int numlabels(0);
	/*vector<bool> touchborders(numlabels, false);*/
	
	DoMeanShiftSegmentation(inputImg, width, height, segimg, sigmaS, sigmaR, minRegion, labels, numlabels);
	//-----------------
	// Form img cluster
	//-----------------
	/////////          int kmax(0);
	/////////          for (int i = 0; i < sz; i++)
	/////////          {
	/////////          	if (kmax < labels[i])
	/////////          		kmax = labels[i];
	/////////          }


	vector<vector<UINT>> imgclustering_(numlabels, vector<uint>(sz, 0));
	{int i(0); 
	int j = 0;
	int k = 0;
	for (j=0; j < height; j++)
	{
		for (k = 0; k < width; k++)
		{
			/*vector<int>::iterator it;
			it = imgclustering[labels[i]].begin();
			it = imgclustering[labels[i]].insert(it+i, 1);*/
			imgclustering_[labels[i]][i] = 255;
			/*if (false == touchborders[labels[i]] && (j == height - 1 || j == 0 || k == width - 1 || k == 0))
			{
				touchborders[labels[i]] = true;
			}*/
			i++;
		}
	}
	}

	//       ofstream Fs("D:\\test1.xls");
	//       if (!Fs.is_open())
	//       {
	//       	cout << "error!" << endl;
	//       	return;
	//       }
	//       
	//       
	//       {int ii(0);
	//       
	//       for (int i = 0; i<height; i++)
	//       {
	//       	for (int j = 0; j<width; j++)
	//       	{
	//       		Fs << imgclustering_[2][ii] << '\t';
	//       	}
	//       	Fs << endl;
	//       }
	//       
	//       }
	//       Fs.close();
	imgclustering = imgclustering_;
	//-----------------
	// Choose segments
	//-----------------
	vector<bool> choose(0);
	ChooseSalientPixelsToShow(salmap, width, height, labels, numlabels, choose);
	//-----------------------------------------------------------------------------
	// Take up only those pixels that are allowed by finalPixelMap
	//-----------------------------------------------------------------------------
	segobj.resize(sz, 0);
	for( int p = 0; p < sz; p++ )
	{
		if( choose[p] )
		{
			segobj[p] = inputImg[p];
		}
	}
}


//===========================================================================
///	OnBnClickedButtonDetectSaliency
///
///	The main function
//===========================================================================
void CSalientRegionDetectorDlg::OnBnClickedButtonDetectSaliency()
{
	PictureHandler picHand;
	vector<string> picvec(0);
	picvec.resize(0);
	string saveLocation = "./data/";                             // ★注意：一定得自己提前创建data输出保存文件夹，否则会报错
	// BrowseForFolder(saveLocation);
	//_CrtSetBreakAlloc(269);                                    //for locating memory leeking;
	GetPictures(picvec);                                         // 用户自己选择图像或图像序列

	int numPics( picvec.size() );                                // 图像序列中图像个数

	for( int k = 0; k < numPics; k++ )
	{
		vector<UINT> img(0);// or UINT* imgBuffer;
		int width(0);
		int height(0);

		picHand.GetPictureBuffer( picvec[k], img, width, height );   // 复制并获取图像信息
		int sz = width*height;                                       // size: 总像素点数

		Saliency sal;
		vector<double> salmap(0);                                    // 初始化显著性图
		sal.GetSaliencyMap(img, width, height, salmap, true);        // 输入原图, 宽度, 高度, 输出显著性图, 进行归一化操作
		
		vector<UINT> outimg(sz);                                     // 计算并输出保存显著性图outimg
		for( int i = 0; i < sz; i++ )
		{
			int val = salmap[i] + 0.5;                                    // 先四舍五入
			outimg[i] = val << 16 | val << 8 | val;                       // 拼接数据：按位左移16位；按位左移8位；原图
		}
		picHand.SavePicture(outimg, width, height, picvec[k], saveLocation, 1, "_1_SalMap");// 0 is for BMP and 1 for JPEG)
		
		//if(m_segmentationflag)          // 注释掉此判断，为了不check也能计算均值漂移
		//{
			vector<UINT> segimg, segobj;
			vector<vector<UINT>> imgclustering;
			DoMeanShiftSegmentationBasedProcessing(img, width, height, picvec[k], salmap, 7, 10, 20, segimg, segobj, imgclustering);
			                                  // 输入图，宽，高，文件名，显著图，sigmaS，sigmaR，minRegion
			cout << imgclustering[0][2] << endl;            // 显示聚类获得的个数
		//	DrawContoursAroundSegments(segimg, width, height, 0xffffff);       //在分割边界画线，以示区别
			//DrawContoursAroundSegments(segobj, width, height, 0xffffff);
			picHand.SavePicture(segimg, width, height, picvec[k], saveLocation, 1, "_2_MeanShift");        // 保存均值漂移图
			picHand.SavePicture(segobj, width, height, picvec[k], saveLocation, 1, "_3_SalientObject");    // 保存显著目标图
		//}



		// 给显著性区域加绿框
		namedWindow("test");
		Mat dest2 = Mat(height, width, CV_8UC4, img.data());               // 初始化加绿框图

		for (int flag = 0; flag < (imgclustering.size()-1); flag++)      // flag表示逐个聚类计算绿框
		{
			vector<cv::Point> points;          // 初始化点集
			int i = 0;
			for (i = 0; i < sz;)
		{   
			int j = 0;
			int k = 0;
			  for (j = 0; j < height; j++)
			  {
				for (k = 0; k < width; k++)
				{
					cv::Point point;           // 定义点
					if (imgclustering[flag][i]>0)       // imgclustering表示图像聚类结果：[聚类编号][像素点编号]
					{
						point.x = k;
						point.y = j;
						points.push_back(point);
					}
					i++;
				 }
			  }
			}
			cv::Rect box = boundingRect(points);
			if (points.size()>(0.005*sz) && points.size()<(0.5*sz))       // 判断点数占总像素点的多大后才会选择画绿框
			{   // 代码改进说明：改进了框中的优胜项，小的框和过大的框不再显示
				rectangle(dest2, box.tl(), box.br(), Scalar(0, 255, 0));
				imshow("test", dest2);                                   // 逐个画绿框
				waitKey(1);
			}
				
		}
		string path,tempStr;                                       // 构造保存路径与保存名称
		tempStr = picvec[k];
		tempStr.erase(tempStr.end() - 4, tempStr.end());    // 获取文件名
		path = tempStr+ "_4_LvKuang.jpg";
		//char* filename;
		//strcpy(filename, path.c_str());
		imwrite(path, dest2);                                        // 保存带绿框的图
	
		

	}
	AfxMessageBox(L"Done!", 0, 0);
}
