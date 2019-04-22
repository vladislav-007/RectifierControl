#pragma once


// SetParametersDialog dialog

class SetParametersDialog : public CDialogEx
{
	DECLARE_DYNAMIC(SetParametersDialog)

public:
	SetParametersDialog(CFont * font, float voltage, float current, CWnd* pParent = NULL);
	virtual ~SetParametersDialog();
	float getVoltage();
	float getCurrent();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SET_PARAMETERS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CFont *m_pFont;
	float m_Voltage;
	float m_Current;
	static WCHAR m_buffer[128];
public:
	afx_msg void OnEnChangeEditV();
	afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult);
};
