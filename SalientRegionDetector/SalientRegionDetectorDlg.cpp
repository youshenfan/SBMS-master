
// SalientRegionDetectorDlg.cpp : implementation file
//
//===========================================================================
// This code implements the saliency detection and segmentation method described in:
//
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
///	GetPictures
///
///	This function collects all the pictures the user chooses into a vector.
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
///	DoMeanShiftSegmentation
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
		bytebuff[i+0] = inputImg[p] >> 16 & 0xff;
		bytebuff[i+1] = inputImg[p] >>  8 & 0xff;
		bytebuff[i+2] = inputImg[p]       & 0xff;
		i += 3;
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
///
///	Do the segmentation of salient region based on K-Means segmentation
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
	string saveLocation = "./data/";
	// BrowseForFolder(saveLocation);
	//_CrtSetBreakAlloc(269); //for locating memory leeking;
	GetPictures(picvec);//user chooses one or more pictures

	int numPics( picvec.size() );

	for( int k = 0; k < numPics; k++ )
	{
		vector<UINT> img(0);// or UINT* imgBuffer;
		int width(0);
		int height(0);

		picHand.GetPictureBuffer( picvec[k], img, width, height );
		int sz = width*height;

		Saliency sal;
		vector<double> salmap(0);
		sal.GetSaliencyMap(img, width, height, salmap, true);

		vector<UINT> outimg(sz);
		for( int i = 0; i < sz; i++ )
		{
			int val = salmap[i] + 0.5;
			outimg[i] = val << 16 | val << 8 | val;
		}
		picHand.SavePicture(outimg, width, height, picvec[k], saveLocation, 1, "_salmap.jpg");// 0 is for BMP and 1 for JPEG)
		
		//if(m_segmentationflag)
		//{
			vector<UINT> segimg, segobj;
			vector<vector<UINT>> imgclustering;
			DoMeanShiftSegmentationBasedProcessing(img, width, height, picvec[k], salmap, 7, 10, 20, segimg, segobj, imgclustering);
			cout << imgclustering[0][2] << endl;
		//	DrawContoursAroundSegments(segimg, width, height, 0xffffff);
			//DrawContoursAroundSegments(segobj, width, height, 0xffffff);
			picHand.SavePicture(segimg, width, height, picvec[k], saveLocation, 1, "_meanshift.jpg");
			picHand.SavePicture(segobj, width, height, picvec[k], saveLocation, 1, "_salientobject.jpg");
		//}


		
		// 加绿框
		namedWindow("test");
		Mat dest2 = Mat(height, width, CV_8UC4, img.data());
	
	

		for (int flag = 0; flag < (imgclustering.size()-1); flag++)
		{
			//int flag = 50;
			vector<cv::Point> points;
			int i = 0;
			for (i = 0; i < height*width;)
		{   
			int j = 0;
			int k = 0;
			  for (j = 0; j < height; j++)
			  {
				for (k = 0; k < width; k++)
				{
					cv::Point point;
					if (imgclustering[flag][i]>0)
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
			//rectangle(dest2, cv::Point(box.x, box.y), cv::Point(box.x + box.width, box.y + box.height), Scalar(0, 255, 0));  // 加绿框
			rectangle(dest2, box.tl(), box.br(), Scalar(0, 255, 0));  // 加绿框
			imshow("test", dest2);
			waitKey(10);
		}
		string path,tempStr;
		tempStr = picvec[k];
		tempStr.erase(tempStr.end() - 4, tempStr.end());
		path = tempStr + "_lvkuang.jpg";
		//char* filename;
		//strcpy(filename, path.c_str());
		imwrite(path, dest2);
	


		// 下一步：改进框中的优胜项；
		// 自动保存在文件夹中


	//imwrite("D:\\basevisual\\SalientRegionDetectorAndSegmenter3\\data\\11.jpg", dest);
	

	}
	AfxMessageBox(L"Done!", 0, 0);
}
