// cncload.h : main header file for the CNCLOAD application
//

#if !defined(AFX_CNCLOAD_H__74851250_0F2F_4C17_A1D8_8D39820CC2DA__INCLUDED_)
#define AFX_CNCLOAD_H__74851250_0F2F_4C17_A1D8_8D39820CC2DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "afxmt.h"


#include "MainFrm.h"

#include "StateDialog.h"


const WORD PORT_CLOSED=0;
const WORD PORT_OPEN=1;
const WORD PORT_SETUPED=2;


struct SThread_param{
	HWND wnd;
	HANDLE * portPtr;
	WORD * statePtr;
	CCriticalSection * portBlock;
	DWORD * dwpByteTimeOut;
	//OVERLAPPED * Sync;
	}; 

/////////////////////////////////////////////////////////////////////////////
// CCncloadApp:
// See cncload.cpp for the implementation of this class
//

class CCncloadApp : public CWinApp
{
public:
	long loadCNCProgram(LPCSTR fileName, BYTE machineNumber);
	SECURITY_ATTRIBUTES port_security_attributes;
	CMainFrame * pMainFrame;
//	long loadCNCProgram( LPCTSTR fileName, BYTE machineNumber );
	//DWORD dwPortNumber;
	DWORD dwWaitTimeOut;
	DWORD dwByteTimeOut;
	WORD getPortState();
	WORD setPortState( WORD aState );
	CCriticalSection portBlock;
	void setPort( HANDLE port );
	char port_name[16];
	CCncloadApp();
	DCB comport_dcb;
	WORD wState;
protected:
	HANDLE * portPtr;
	HANDLE port;
	CStateDialog * stateDlg;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCncloadApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CCncloadApp)
	afx_msg void OnAppAbout();
	afx_msg void OnOptionComport();
	afx_msg void OnMenuOptionStatesWnd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNCLOAD_H__74851250_0F2F_4C17_A1D8_8D39820CC2DA__INCLUDED_)
