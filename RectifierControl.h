
// RectifierControl.h : ãëàâíûé ôàéë çàãîëîâêà äëÿ ïðèëîæåíèÿ RectifierControl
//
#ifndef RECTIFIERCONTROL_H 
#define RECTIFIERCONTROL_H 
#pragma once


#ifndef __AFXWIN_H__
	#error "âêëþ÷èòü stdafx.h äî âêëþ÷åíèÿ ýòîãî ôàéëà â PCH"
#endif

#include "resource.h"       // îñíîâíûå ñèìâîëû
#include "RectifierCommand.h"
#include "RectifierControlDoc.h"

#include <map>
#include <vector>
#include "scomport.h"

// CRectifierControlApp:
// Î ðåàëèçàöèè äàííîãî êëàññà ñì. RectifierControl.cpp
//




//struct RectifierInfo {
//	int id;
//	CString name;
//	int address;
//	CString comport;
//	int modeID;
//	CString modeName;
//	int modeBoundRate;
//	int modeByteSize;
//	Parity modeParity;
//	Stopbits modeStopbits;
//};

typedef struct _CMDFRAME {
	static const uint8_t BEGIN = ':';
	uint8_t MODBUS_ADDR;
	uint8_t MODBUS_FUNC;
	std::vector<uint8_t> data;
	uint8_t LRC;
	static const uint8_t RC = 0x0D;
	static const uint8_t LF = 0x0A;
} CMDFRAME, *LPCMDFRAME;


std::vector<uint8_t> convertToASCIIFrame(const std::vector<uint8_t> & frame_bytes);
Parity parityFromString(const CString & parityStr);
Stopbits stopbitsFromString(const CString & stopBitsStr);





class CRectifierControlApp : public CWinApp
{
public:
	CRectifierControlApp();


// Ïåðåîïðåäåëåíèå
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Ðåàëèçàöèÿ
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLinkOptions();
private:
	CString m_usedComPort;
	SECURITY_ATTRIBUTES port_security_attributes;
	//CMainFrame * m_pMainFrame;
	// Properties of connection port. 
	std::map<CString, COMMCONFIG> m_comportProperties;
	std::map<int, RectifierInfo> m_rectifierConfigs;
	WORD m_threadState;
public:
	void registerRectifier(CRectifierControlDoc * rectifierDoc);
	afx_msg void OnRectifierState();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	afx_msg void OnFileOpen();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
};

extern CRectifierControlApp theApp;

#endif