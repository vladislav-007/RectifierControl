// RectifiersStateDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RectifierControl.h"
#include "RectifiersStateDialog.h"
#include "afxdialogex.h"
#include <sstream>
#include <iostream>
#include "RectifierCommand.h"

OVERLAPPED overlappedWR;
OVERLAPPED overlappedRD;

// CRectifiersStateDialog dialog

IMPLEMENT_DYNAMIC(CRectifiersStateDialog, CDialogEx)

CRectifiersStateDialog::CRectifiersStateDialog(std::map<int, RectifierInfo> & rectifierConfigs, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_RECTIFIERS_STATE_DIALOG, pParent), m_rectifierConfigs(rectifierConfigs)
{

}

CRectifiersStateDialog::~CRectifiersStateDialog()
{
}

void CRectifiersStateDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDRESS_TEXT, m_addressText);
	DDX_Control(pDX, IDC_TEST_LOG, m_CEditTestLog);
}


BEGIN_MESSAGE_MAP(CRectifiersStateDialog, CDialogEx)
	ON_CBN_SELCHANGE(IDC_RECTIFIERS_COMBO, &CRectifiersStateDialog::OnCbnSelchangeRectifiersCombo)
	ON_BN_CLICKED(IDC_BUTTON1, &CRectifiersStateDialog::OnBnClickedButton1)
END_MESSAGE_MAP()


// CRectifiersStateDialog message handlers

BOOL CRectifiersStateDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_RECTIFIERS_COMBO);
	if (pComboBox != nullptr) {
		for (auto const & item : m_rectifierConfigs) {
			const RectifierInfo & info = item.second;
			CString id = CString();
			id.Format(L"%d : (%s)", info.id, info.name);
			pComboBox->AddString(id);
		}
		pComboBox->SetCurSel(0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void CRectifiersStateDialog::OnCbnSelchangeRectifiersCombo()
{
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_RECTIFIERS_COMBO);
	if (pComboBox != nullptr) {
		CString str;
		pComboBox->GetLBText(pComboBox->GetCurSel(), str);
		int dwPos = str.Find(L" :", 0);
		if (dwPos != -1)
			str = str.Left(dwPos);
		int id = _wtoi(str);
		RectifierInfo & info = m_rectifierConfigs.at(id);
		CString address;
		address.Format(L"%d", info.address);
		m_addressText.SetWindowText(address);
	}
	
}


void CRectifiersStateDialog::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_RECTIFIERS_COMBO);
	if (pComboBox != nullptr) {
		CString str;
		pComboBox->GetLBText(pComboBox->GetCurSel(), str);
		int dwPos = str.Find(L" :", 0);
		if (dwPos != -1)
			str = str.Left(dwPos);
		int id = _wtoi(str);
		RectifierInfo & info = m_rectifierConfigs.at(id); 
		CString log;
		m_CEditTestLog.GetWindowText(log);
		log += L"Send command 0x07";

		m_CEditTestLog.SetWindowText(log);
		std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
			info.address, 0x43, GET_CONCISE_DEVICE_STATE_07);

		frameBytes = DeviceCommand::convertToASCIIFrame(
			frameBytes);
		
		HANDLE hSerial;
		LPCTSTR sPortName = info.comport;
		hSerial = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if (hSerial == INVALID_HANDLE_VALUE)
		{
			CString message;
			message.Format(L"Failed to open comport %s.", info.comport);
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				message += L"Comport doesn't exists";
			}
			AfxMessageBox(message, MB_YESNO | MB_ICONSTOP);
		}
		DCB dcbSerialParams = { 0 };
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
		if (!GetCommState(hSerial, &dcbSerialParams))
		{
			AfxMessageBox(L"getting state error", MB_YESNO | MB_ICONSTOP);
		}
		dcbSerialParams.BaudRate = info.modeBoundRate;
		dcbSerialParams.ByteSize = info.modeByteSize;
		dcbSerialParams.StopBits = (BYTE)info.modeStopbits;
		dcbSerialParams.Parity = (BYTE)info.modeParity;
		if (!SetCommState(hSerial, &dcbSerialParams))
		{
			log += L"error setting serial port state\n";
			m_CEditTestLog.SetWindowText(log);
			AfxMessageBox(L"getting state error", MB_YESNO | MB_ICONSTOP);
		}
		const uint8_t * data = frameBytes.data();  // строка для передачи
		DWORD dwSize = frameBytes.size();   // размер этой строки
		DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
		overlappedWR.hEvent = CreateEvent(NULL, true, true, NULL);

		unsigned char bufrd[1024];
		overlappedRD.hEvent = CreateEvent(NULL, true, true, NULL);
		SetCommMask(hSerial, EV_RXCHAR);
		DWORD mask;
		//ожидать события приёма байта (это и есть перекрываемая операция)
		WaitCommEvent(hSerial, &mask, &overlappedRD);


		BOOL iRet = WriteFile(hSerial, data, dwSize, &dwBytesWritten, &overlappedWR);
		DWORD btr, temp, signal;
		signal = WaitForSingleObject(overlappedWR.hEvent, 1000);	//приостановить поток, пока не завершится
																		//перекрываемая операция WriteFile
																		//если операция завершилась успешно, установить соответствующий флажок
		std::wstringstream ss;
		if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hSerial, &overlappedWR, &dwBytesWritten, true))) {
			ss << L"sent - OK \n/n" << std::endl;
		}
		else {

			ss << L"Failed to send cm..." << std::endl;
		}

		
		ss << dwSize << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
		std::wstring str1;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
		Sleep(150);

		ss.clear();
		DWORD iSize;
		//char sReceivedChar;
		ss << L"Started reading replay..." << std::endl;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
		COMSTAT comstat;
		//OVERLAPPED overlapped;
		while (true)
		{
			
			signal = WaitForSingleObject(overlappedRD.hEvent, 10000);	//приостановить поток до прихода байта
			if (signal == WAIT_OBJECT_0)				        //если событие прихода байта произошло
			{
				if (GetOverlappedResult(hSerial, &overlappedRD, &temp, true)) //проверяем, успешно ли завершилась
																			//перекрываемая операция WaitCommEvent
					if ((mask & EV_RXCHAR) != 0)				//если произошло именно событие прихода байта
					{
						ClearCommError(hSerial, &temp, &comstat);		//нужно заполнить структуру COMSTAT
						btr = comstat.cbInQue;                          	//и получить из неё количество принятых байтов
						if (btr)                         			//если действительно есть байты для чтения
						{
							ReadFile(hSerial, bufrd, btr, &iSize, &overlappedRD);     //прочитать байты из порта в буфер программы
							if (iSize > 0) {   // если что-то принято, выводим
								for( DWORD i=0; i < iSize;++i)
									ss << std::hex << bufrd[i];
							}
						}
					}
			}else {
				log += L"НЕТ ОТВЕТА!!!";
				m_CEditTestLog.SetWindowText(log);
				AfxMessageBox(L"НЕТ ОТВЕТА", MB_YESNO | MB_ICONSTOP);
				break;
			}
			break;
		}
		ss << std::endl;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
		CloseHandle(hSerial);
	}
}


