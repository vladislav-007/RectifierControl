#pragma once


// CSetComportDlg dialog

class CSetComportDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSetComportDlg)

public:
	CSetComportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSetComportDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SET_COMPORT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CString m_usedComPort;
public:
	void setCurrentlyUsedComport(CString usedComPort);
	CString getComport();
	virtual void OnOK();
};
