// SetParametersDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RectifierControl.h"
#include "SetParametersDialog.h"
#include "afxdialogex.h"
#include <string>


// SetParametersDialog dialog

IMPLEMENT_DYNAMIC(SetParametersDialog, CDialogEx)

WCHAR SetParametersDialog::m_buffer[128] = {};
//const WCHAR SetParametersDialog::buffer = _data;

SetParametersDialog::SetParametersDialog(CFont * font, float voltage, float current, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SET_PARAMETERS_DIALOG, pParent)
{
	m_pFont = font;
	m_Voltage = voltage;
	m_Current = current;
}

SetParametersDialog::~SetParametersDialog()
{
}

float SetParametersDialog::getVoltage()
{
	return m_Voltage;
}

float SetParametersDialog::getCurrent()
{
	return m_Current;
}

void SetParametersDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_V, m_Voltage);
	DDX_Text(pDX, IDC_EDIT_A, m_Current);
}


BEGIN_MESSAGE_MAP(SetParametersDialog, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT_V, &SetParametersDialog::OnEnChangeEditV)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, &SetParametersDialog::OnDeltaposSpin1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN2, &SetParametersDialog::OnDeltaposSpin2)
END_MESSAGE_MAP()


// SetParametersDialog message handlers


BOOL SetParametersDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  Add extra initialization here
	CWnd* pChild = GetWindow(GW_CHILD);
	while (pChild) {
		pChild->SetFont(m_pFont, TRUE);
		pChild = pChild->GetNextWindow(); 
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void SetParametersDialog::OnEnChangeEditV()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code he
	//CString input;
	//GetDlgItemText(IDC_EDIT_V, input);
	//double d = std::stod(std::wstring(input));
	//CString str;
	//str.Format(L"%f", d);
	//_tcscpy_s((wchar_t *)m_buffer, _countof(m_buffer), input);
	//SetDlgItemText(IDC_EDIT_V, m_buffer);
}


void SetParametersDialog::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	m_Voltage -= pNMUpDown->iDelta;
	if (m_Voltage < 0) {
		m_Voltage = 0;
	}
	if (m_Voltage > 100) {
		m_Voltage = 100;
	}
	*pResult = 0;
	UpdateData(FALSE);
}


void SetParametersDialog::OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	m_Current -= 10 * pNMUpDown->iDelta;
	if (m_Current < 0) {
		m_Current = 0;
	}
	if (m_Current > 999) {
		m_Current = 999;
	}
	*pResult = 0;
	UpdateData(FALSE);
}
