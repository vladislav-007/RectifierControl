
// RectifierControl.h : ������� ���� ��������� ��� ���������� RectifierControl
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�������� stdafx.h �� ��������� ����� ����� � PCH"
#endif

#include "resource.h"       // �������� �������


// CRectifierControlApp:
// � ���������� ������� ������ ��. RectifierControl.cpp
//

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
};

extern CRectifierControlApp theApp;
