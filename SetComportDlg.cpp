// SetComportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RectifierControl.h"
#include "SetComportDlg.h"
#include "afxdialogex.h"

#include <vector>
#include <string>


// CSetComportDlg dialog

IMPLEMENT_DYNAMIC(CSetComportDlg, CDialogEx)

CSetComportDlg::CSetComportDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SET_COMPORT_DIALOG, pParent)
	, m_usedComPort(_T(""))
{

}

CSetComportDlg::~CSetComportDlg()
{
}

void CSetComportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSetComportDlg, CDialogEx)
END_MESSAGE_MAP()


// CSetComportDlg message handlers

void SelectComPort(std::vector<CString> & portNames) //added fucntion to find the present serial ports of the system; 
{
	const DWORD maxSize = 50000;
	WCHAR lpTargetPath[maxSize]; // buffer to store the path of the COMPORTS
	DWORD test;
	bool gotPort = 0; // in case the port is not found

	for (int i = 0; i<255; i++) // checking ports from COM0 to COM255
	{
		CString str;
		str.Format(_T("%d"), i);
		CString ComName = CString("COM") + CString(str); // converting to COM0, COM1, COM2

		test = QueryDosDevice(ComName, (LPWSTR)lpTargetPath, maxSize);

		// Test the return value and error if any
		if (test != 0) //QueryDosDevice returns zero if it didn't find an object
		{
			portNames.push_back((CString)ComName); // add to the ComboBox
			gotPort = 1; // found port
		}
		DWORD res = ::GetLastError();
		res = ::GetLastError();
		if (res == ERROR_INSUFFICIENT_BUFFER) //in case buffer got filled
		{
			lpTargetPath[10000]; // in case the buffer got filled, increase size of the buffer.
			continue;
		}

	}

	if (!gotPort) // if not port
		portNames.push_back((CString)"No Active Ports Found"); // to display error message incase no ports found

}



BOOL CSetComportDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	std::vector<CString> names;
	SelectComPort(names);
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_COMPORT);
	if (pComboBox != nullptr) {
		int i = 0;
		int j = -1;
		for (CString name : names) {
			pComboBox->AddString(name);
			if (m_usedComPort == name) {
				pComboBox->SetCurSel(i);
				j = i;
			}
			++i;
		}
		if (j < 0)
			pComboBox->SetCurSel(0);
	}


	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void CSetComportDlg::setCurrentlyUsedComport(CString usedComPort)
{
	m_usedComPort = usedComPort;
}


CString CSetComportDlg::getComport()
{
	return m_usedComPort;
}


void CSetComportDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_COMPORT);
	if (pComboBox != nullptr) {
		pComboBox->GetLBText(pComboBox->GetCurSel(), m_usedComPort);
	}
	CDialogEx::OnOK();
}
