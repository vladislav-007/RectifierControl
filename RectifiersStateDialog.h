#pragma once
#include "afxwin.h"


// CRectifiersStateDialog dialog

class CRectifiersStateDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CRectifiersStateDialog)

public:
	CRectifiersStateDialog(std::map<int, RectifierInfo> & m_rectifierConfigs, CWnd* pParent = NULL);   // standard constructor
	virtual ~CRectifiersStateDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RECTIFIERS_STATE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	//void SetRectifiersConfigs(const std::map<int, RectifierInfo> & rectifiersState);
	std::map<int, RectifierInfo> & m_rectifierConfigs;
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeRectifiersCombo();
private:
	CStatic m_addressText;
	int state;
public:
	afx_msg void OnBnClickedButton1();
	CEdit m_CEditTestLog;
	CString m_log;
	afx_msg void OnBnClickedButton2();
	static void modelateRectifier(OVERLAPPED * const stateDialogOverlappedRD, DWORD * pMask, OVERLAPPED * const stateDialogOverlappedWR,
		RectifierInfo & info, CEdit & m_CEditTestLog, CString & log, int * state);
	afx_msg void OnBnClickedButton3();
	virtual void OnCancel();
	afx_msg void OnClose();
	afx_msg void OnBnClickedCheck1();
};

UINT StateThreadProc(LPVOID par);
