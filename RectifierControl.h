
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

#include <map>
#include <vector>


// CRectifierControlApp:
// Î ðåàëèçàöèè äàííîãî êëàññà ñì. RectifierControl.cpp
//
enum class Parity : std::int8_t {
	NO_PARITY = 0,
	ODD_PARITY = 1,
	EVEN_PARITY = 2,
	MARK_PARITY = 3,
	SPACE_PARITY = 4
};





enum class Stopbits : std::int8_t {
	ONE_STOPBIT = 0,
	ONE5_STOPBITS = 1,
	TWO_STOPBITS = 2
};




struct RectifierInfo {
	int id;
	CString name;
	int address;
	CString comport;
	int modeID;
	CString modeName;
	int modeBoundRate;
	int modeByteSize;
	Parity modeParity;
	Stopbits modeStopbits;
};

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
	// Properties of connection port. 
	std::map<CString, COMMCONFIG> m_comportProperties;
	std::map<int, RectifierInfo> m_rectifierConfigs;
public:
	afx_msg void OnRectifierState();
};

extern CRectifierControlApp theApp;

#endif