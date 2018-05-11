
// RectifierControl.h : ������� ���� ��������� ��� ���������� RectifierControl
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�������� stdafx.h �� ��������� ����� ����� � PCH"
#endif

#include "resource.h"       // �������� �������

#include <map>


// CRectifierControlApp:
// � ���������� ������� ������ ��. RectifierControl.cpp
//
struct RectifierInfo;

class CRectifierControlApp : public CWinApp
{
public:
	CRectifierControlApp();


// ���������������
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ����������
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLinkOptions();
private:
	CString m_usedComPort;
	// Properties of connection port. 
	std::map<CString, COMMCONFIG> m_comportProperties;
	std::map<int, RectifierInfo> m_rectifierConfigs;
};

extern CRectifierControlApp theApp;
