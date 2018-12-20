// RectifiersStateDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RectifierControl.h"
#include "RectifiersStateDialog.h"
#include "afxdialogex.h"
#include <sstream>
#include <iostream>
#include "RectifierCommand.h"
#include "scomport.h"
#include <algorithm>
#include <iterator>  
#include "afxwinappex.h"

OVERLAPPED stateDialogOverlappedWR;
OVERLAPPED stateDialogOverlappedRD;
DWORD mask;
OVERLAPPED stateDialogOverlappedWR1;
DWORD mask1;
OVERLAPPED stateDialogOverlappedRD1;

// CRectifiersStateDialog dialog

IMPLEMENT_DYNAMIC(CRectifiersStateDialog, CDialogEx)

CRectifiersStateDialog::CRectifiersStateDialog(std::map<int, RectifierInfo> & rectifierConfigs, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_RECTIFIERS_STATE_DIALOG, pParent), m_rectifierConfigs(rectifierConfigs)
{
	state = 0;
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
	ON_BN_CLICKED(IDC_BUTTON2, &CRectifiersStateDialog::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CRectifiersStateDialog::OnBnClickedButton3)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CRectifiersStateDialog message handlers

BOOL CRectifiersStateDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CComboBox * pComboBox = (CComboBox*)GetDlgItem(IDC_RECTIFIERS_COMBO);
	if (pComboBox != nullptr) {
		 
//		if (m_rectifierConfigs.empty()) {
			for (int i = 1; i < 10; ++i) {
				CString id = CString();
				id.Format(L"%d : (Test Rectifier COM%d)", i, i);
				pComboBox->AddString(id);
			}
		//}
		//else {
		//	for (auto const & item : m_rectifierConfigs) {
		//		const RectifierInfo & info = item.second;
		//		CString id = CString();
		//		id.Format(L"%d : (%s)", info.id, info.name);
		//		pComboBox->AddString(id);
		//	}
		//}
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
		//if (!m_rectifierConfigs.empty()) {
		//	RectifierInfo & info = m_rectifierConfigs.at(id);
		//	CString address;
		//	address.Format(L"%d", info.address);
		//	m_addressText.SetWindowText(address);
		//}
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
		
		RectifierInfo testRectirierInfo;
		RectifierInfo & info = testRectirierInfo;
		if (m_rectifierConfigs.empty()) {
			CString str;
			str.Format(L"COM%d", id);
			testRectirierInfo.comport = str;
			testRectirierInfo.address = 1;
			testRectirierInfo.modeBoundRate = 115200;
			testRectirierInfo.modeByteSize = 7;
			testRectirierInfo.modeStopbits = Stopbits::ONE_STOPBIT;
			testRectirierInfo.modeParity = Parity::NO_PARITY;
		}
		else {
			info = m_rectifierConfigs.at(id);
		}
		
		CString log;
		m_CEditTestLog.GetWindowText(log);
		log += L"Send command 0x07";
		m_CEditTestLog.SetWindowText(log);

		Device device(testRectirierInfo, &stateDialogOverlappedRD1, &mask1, &stateDialogOverlappedWR1);

		std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
			info.address, 0x43, GET_CONCISE_DEVICE_STATE_07);

		frameBytes = DeviceCommand::convertToASCIIFrame(
			frameBytes);

		DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
		device.sendCommand(frameBytes, dwBytesWritten, log);
		std::wstringstream ss;
		ss << frameBytes.size() << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
		std::wstring str1;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
		Sleep(150);

		ss.clear();
		//DWORD iSize;
		//char sReceivedChar;
		ss << L"Started reading replay..." << std::endl;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
		//COMSTAT comstat;
		//OVERLAPPED overlapped;
		state = 1;
		std::vector<uint8_t> rdSymbols;
		device.readFromPort(rdSymbols);
		for (auto symbol : rdSymbols) {
			ss << std::hex << symbol;
		}
		ss << std::endl;
		str1 = ss.str();
		log += str1.c_str();
		m_CEditTestLog.SetWindowText(log);
	}
}

class RectifierStateF10 {
	std::uint8_t aLow; // current
	std::uint8_t aHi;
	std::uint8_t vLow; // 
	std::uint8_t vHi;
	std::uint8_t configByte;
	std::vector<uint8_t> serialNumber; // serial number 0x11 bytes
};

//std::vector<uint8_t> rectifierStateFrame = { 0x3A, 0x30, 0x31, 0x34, 0x33, 0x45, 0x38, 0x30, 0x33, 0x37, 0x38, 0x30, 0x30, 0x34, 0x31, 0x33, 0x30, 0x33, 0x31, 0x33, 0x31, 0x33, 0x38, 0x32, 0x44, 0x33, 0x33, 0x33, 0x31, 0x33, 0x31, 0x33, 0x32, 0x33, 0x37, 0x33, 0x32, 0x33, 0x34, 0x42, 0x44, 0x0D, 0x0A };

struct ModelateThreadParams {
	HWND wnd;
	int * state;
	CString * log;
	//HANDLE hSerial;
	//CCriticalSection * portBlock;
	//DWORD * dwpByteTimeOut;
	RectifierInfo rectifierConfig;
	CEdit * m_CEditTestLog;
	OVERLAPPED * stateDialogOverlappedRD;
	DWORD * pMask;
	OVERLAPPED * stateDialogOverlappedWR;
};

ModelateThreadParams param;

UINT StateThreadProc(LPVOID par) {
	ModelateThreadParams * param;
	//CSComPort comPort;
	param = (ModelateThreadParams *)par;

	CRectifiersStateDialog::modelateRectifier(param->stateDialogOverlappedRD, param->pMask, param->stateDialogOverlappedWR, param->rectifierConfig, *param->m_CEditTestLog, *param->log, param->state);

	//::MessageBox(NULL, L"Thread stopted", L"Thread", MB_OK);
	param->state[0] = 4;
	AfxEndThread(0);
	return 0;
}


void CRectifiersStateDialog::OnBnClickedButton2()
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

		RectifierInfo testRectirierInfo;
		RectifierInfo & info = testRectirierInfo;
		str.Format(L"COM%d", id);
		testRectirierInfo.comport = str;
		testRectirierInfo.address = 1;
		testRectirierInfo.modeBoundRate = 115200;
		testRectirierInfo.modeByteSize = 7;
		testRectirierInfo.modeStopbits = Stopbits::ONE_STOPBIT;
		testRectirierInfo.modeParity = Parity::NO_PARITY;
	
		//
		//m_CEditTestLog.GetWindowText(m_log);
		//m_CEditTestLog.SetWindowText(m_log);
		//HANDLE hSerial;
		//LPCTSTR sPortName = info.comport;
		//hSerial = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		//if (hSerial == INVALID_HANDLE_VALUE)
		//{
		//	CString message;
		//	message.Format(L"Failed to open comport %s.", info.comport);
		//	if (GetLastError() == ERROR_FILE_NOT_FOUND)
		//	{
		//		message += L"Comport doesn't exists";
		//	}
		//	AfxMessageBox(message, MB_YESNO | MB_ICONSTOP);
		//	return;
		//}

		
		//param.wnd = m_pMainWnd->GetSafeHwnd();
		param.rectifierConfig = testRectirierInfo;
		//param.portPtr = portPtr;
		param.state = &this->state;
		param.log = &m_log;
		//param.hSerial = hSerial;
		//param.portBlock = &portBlock;
		//param.dwpByteTimeOut = &dwByteTimeOut;
		param.m_CEditTestLog = &m_CEditTestLog;
		param.stateDialogOverlappedRD = &stateDialogOverlappedRD;
		param.pMask = &mask;
		param.stateDialogOverlappedWR = &stateDialogOverlappedWR;

		AfxBeginThread(StateThreadProc, &param, THREAD_PRIORITY_NORMAL);

	}

}

void CRectifiersStateDialog::modelateRectifier(
	OVERLAPPED * const stateDialogOverlappedRD,
	DWORD * pMask,
	OVERLAPPED * const stateDialogOverlappedWR,
	RectifierInfo & info, CEdit & m_CEditTestLog, CString & log, int * state) {
	//DCB dcbSerialParams = { 0 };
	//dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	//if (!GetCommState(hSerial, &dcbSerialParams))
	//{
	//	//AfxMessageBox(L"getting state error", MB_YESNO | MB_ICONSTOP);
	//	state[0] = 5;
	//	CloseHandle(hSerial);
	//	return;
	//}
	//dcbSerialParams.BaudRate = info.modeBoundRate;
	//dcbSerialParams.ByteSize = info.modeByteSize;
	//dcbSerialParams.StopBits = (BYTE)info.modeStopbits;
	//dcbSerialParams.Parity = (BYTE)info.modeParity;
	//if (!SetCommState(hSerial, &dcbSerialParams))
	//{
	//	log += L"error setting serial port state\n";
	//	//m_CEditTestLog.SetWindowText(log);
	//	//AfxMessageBox(L"getting state error", MB_YESNO | MB_ICONSTOP);
	//}
	
	
	std::wstringstream ss;
	ss.clear();
	//DWORD iSize;
	//char sReceivedChar;
	ss << L"Started reading commands..." << std::endl;
	std::wstring str1;
	str1 = ss.str();
	log += str1.c_str();
	//m_CEditTestLog.SetWindowText(log);
	//COMSTAT comstat;
	//DWORD temp;
	std::vector<std::uint8_t> rdSymbolsFrame;
	//std::vector<std::uint8_t> symbolsTail;
	std::vector<uint8_t> replyOKBytes = DeviceCommand::createReplyFrame(
		0x01, 0x43, 0x05, ReplyStatus::OK);
	//std::vector<uint8_t> replyOKSymbols = DeviceCommand::convertToASCIIFrame(replyOKBytes);

	state[0] = 1;
	Device device(info, stateDialogOverlappedRD, pMask, stateDialogOverlappedWR);
	while (true)
	{
		device.getFrameFromBuffer(rdSymbolsFrame);
		if (!Device::isValidFrame(rdSymbolsFrame)) {
			device.readFromPort(rdSymbolsFrame);
		}

		if (!Device::isValidFrame(rdSymbolsFrame)) {
			continue;
		}

		// it can be read less than frame or more than a frame ??? 
		// try to get one frame only
		if (!rdSymbolsFrame.empty()) {
			std::vector<std::uint8_t> readBytes = DeviceCommand::parseASCIIFrameToBytes(rdSymbolsFrame);
			if (readBytes.empty())
				continue;
			std::uint8_t addr, modbus_func;

			//std::uint8_t replyCode;
			//DeviceCommand::parseResponseCode(readBytes, addr, modbus_func, replyCode);
			DeviceCommand::DATA cmd_data;
			DeviceCommand::DATA prev_cmd_data;
			DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
			if (0 != DeviceCommand::parseCommand(readBytes, addr, modbus_func, cmd_data))
				//bad command try to get nest frame
				continue;
			if (0x10 == cmd_data.address) {
				// asked for rectifier state. Send normal reply :014305B7 0D 0A
				
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (0x06 == cmd_data.address) {
				// f0601 activate remote panel
				// f0602 activate local control panel
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (0x02 == cmd_data.address) {
				// asked  state. Send normal reply :014305B7 0D 0A
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (0x03 == cmd_data.address) {
				// asked for rectifier state. Send normal reply :014305B7 0D 0A
				Sleep(50);
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (0x05 == cmd_data.address) {
				// asked for rectifier state. Send normal reply :014305B7 0D 0A
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (0x07 == cmd_data.address) {
				// asked for rectifier state. Send normal reply :014305B7 0D 0A
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
				//sendCommand(hSerial, data, 1, dwBytesWritten, m_CEditTestLog, log);
			}
			else if (cmd_data.address == 0x01) {
				//asked for data itself
				if (0x10 == prev_cmd_data.address) {
					//std::vector<uint8_t> serialNumber = { 1,2,3,4,5,6,7,8,9,0,1,2 };
					//std::vector<uint8_t> reply_data = DeviceCommand::createRectifierInfoF10(3, 5, 7, 9, 0x11, serialNumber);
					//std::vector<uint8_t> replyBytes = DeviceCommand::createReplyDataFrame(
					//	info.address, 0x43, reply_data);
					//std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(replyBytes);
					////sendReply(hSerial, frameSymbols, dwBytesWritten, m_CEditTestLog, log);
					std::vector<uint8_t> rectifierStateFrame = { 0x3A, 0x30, (uint8_t)(addr + 0x30), 0x34, 0x33, 0x45, 0x38, 0x30, 0x33, 0x37, 0x38, 0x30, 0x30, 0x34, 0x31, 0x33, 0x30, 0x33, 0x31, 0x33, 0x31, 0x33, 0x38, 0x32, 0x44, 0x33, 0x33, 0x33, 0x31, 0x33, 0x31, 0x33, 0x32, 0x33, 0x37, 0x33, 0x32, 0x33, 0x34, 0x42, 0x44, 0x0D, 0x0A };
					device.sendReplyData(rectifierStateFrame, dwBytesWritten, log);
				}
				if (0x07 == prev_cmd_data.address) {
					//std::vector<uint8_t> serialNumber = { 1,2,3,4,5,6,7,8,9,0,1,2 };
					static uint8_t v = 0x78;
					if (v < 0x250) {
						v = v + 1;
					}
					else {
						v = 0x20;
					}

					std::vector<uint8_t> reply_data = DeviceCommand::createRectifierStateF07(3, 0x11, 0x11, 0, 0, 0xE8, 0x03, v);
					std::vector<uint8_t> replyBytes = DeviceCommand::createReplyDataFrame(addr, 0x43, reply_data);
					std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(replyBytes);
					//std::vector<uint8_t> rectifierStateFrame = { 0x3A, 0x30, 0x31, 0x34, 0x33, 0x45, 0x38, 0x30, 0x33, 0x37, 0x38, 0x30, 0x30, 0x34, 0x31, 0x33, 0x30, 0x33, 0x31, 0x33, 0x31, 0x33, 0x38, 0x32, 0x44, 0x33, 0x33, 0x33, 0x31, 0x33, 0x31, 0x33, 0x32, 0x33, 0x37, 0x33, 0x32, 0x33, 0x34, 0x42, 0x44, 0x0D, 0x0A };
					device.sendReplyData(frameSymbols, dwBytesWritten, log);
				}
				else if (0x05 == prev_cmd_data.address) {
					static uint8_t v = 0x78;
					if (v < 0x250) {
						v = v + 1;
					}
					else {
						v = 0x20;
					}
					std::vector<uint8_t> reply_data = DeviceCommand::createRectifierStateF05(1, 2, 3, 3, 0x11, 0x11, 0, 0, addr, 0x0, v);
					std::vector<uint8_t> replyBytes = DeviceCommand::createReplyDataFrame(addr, 0x43, reply_data);
					std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(replyBytes);
					//std::vector<uint8_t> deviceStateFrame = { 0x3A,0x30,(uint8_t)(addr + 0x30),0x34,0x33,0x46,0x46,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x44,0x41,0x30,0x36,0x30,0x30,0x30,0x30,0x30,0x31,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x38,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x35,0x35,0x41,0x41,0x34,0x43,0x0D,0x0A };
					device.sendReplyData(frameSymbols, dwBytesWritten, log);
				}
				else if (0x03 == prev_cmd_data.address && 0x00 == prev_cmd_data.command) {
					if (cmd_data.data[0] == 0x00 && cmd_data.data[1] == 0x00 && cmd_data.data[2] == 0x20) {
						std::vector<uint8_t> deviceStateOn05Frame = { 0x3A,0x30,0x31,0x34,0x33,0x46,0x46,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x44,0x41,0x30,0x36,0x30,0x30,0x30,0x30,0x30,0x31,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x38,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x35,0x35,0x41,0x41,0x34,0x43,0x0D,0x0A };
						device.sendReplyData(deviceStateOn05Frame, dwBytesWritten, log);
					}
					else if (cmd_data.data[0] == 0x20 && cmd_data.data[1] == 0x00 && cmd_data.data[2] == 0x20) {
						std::vector<uint8_t> deviceMemoryFrom20 = { 0x3A,0x30,0x31,0x34,0x33,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x42,0x43,0x0D,0x0A };
						device.sendReplyData(deviceMemoryFrom20, dwBytesWritten, log);
					}
					else {
						std::vector<uint8_t> deviceMemoryFromAnyOther = { 0x3A,0x30,0x31,0x34,0x33,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x42,0x43,0x0D,0x0A };
						device.sendReplyData(deviceMemoryFromAnyOther, dwBytesWritten, log);
					}
				}
				else {
					std::vector<uint8_t> deviceStateFrame = { 0x3A,0x30,0x31,0x34,0x33,0x46,0x46,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x44,0x41,0x30,0x36,0x30,0x30,0x30,0x30,0x30,0x31,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x38,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x35,0x35,0x41,0x41,0x34,0x43,0x0D,0x0A };
					device.sendReplyData(deviceStateFrame, dwBytesWritten, log);
				}
			}
			else {
				device.sendReplyData(DeviceCommand::createReplyOKFrameSymbols(addr), dwBytesWritten, log);
			}

			prev_cmd_data.address = cmd_data.address;
			prev_cmd_data.command = cmd_data.command;
			prev_cmd_data.data = cmd_data.data;

		}
		if (state[0] == 2) {
			break;
		}

	}
	ss << std::endl;
	str1 = ss.str();
	log += str1.c_str();
	state[0] = 3;
}

void CRectifiersStateDialog::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	if (this->state == 1) {
		this->state = 2; //exit modulate mode
	}
}





void CRectifiersStateDialog::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	while (state <= 3) {
		state = 2;
		Sleep(100);
	}
	CDialogEx::OnCancel();
}


void CRectifiersStateDialog::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnClose();
}
