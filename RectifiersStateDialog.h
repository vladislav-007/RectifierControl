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
public:
	afx_msg void OnBnClickedButton1();
	CEdit m_CEditTestLog;
	afx_msg void OnBnClickedButton2();
};
