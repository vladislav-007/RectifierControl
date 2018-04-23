
// RectifierControl.h : главный файл заголовка для приложения RectifierControl
//
#pragma once

#ifndef __AFXWIN_H__
	#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы


// CRectifierControlApp:
// О реализации данного класса см. RectifierControl.cpp
//

class CRectifierControlApp : public CWinApp
{
public:
	CRectifierControlApp();


// Переопределение
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Реализация
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CRectifierControlApp theApp;
