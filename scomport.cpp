#include "stdafx.h"
#include "scomport.h"
#include "RectifierCommand.h"
#include <vector>
#include <cmath>

//#include "cncload.h"

#include <Windows.h>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;
//OVERLAPPED overlappedWR;
//OVERLAPPED overlappedRD;

std::map<CString, HANDLE> Device::openedPorts;
std::map<HANDLE, int> Device::openedPortsCount;
CCriticalSection RectifierInfo::cs;

char * taskStateStr[] = { ("Нет задания"),"Ожидание запроса на загрузку программы",
	"Загрузка программы",
	"Загрузка программы завершена", "","" };

char * machStateStr[] = { " Режим ожидания","Старт фотоввода","", NULL };

std::vector<std::uint8_t> Device::getFrameFromTail(std::vector<std::uint8_t> & symbolsTail) {
	if (symbolsTail.empty())
		return symbolsTail;

	std::vector<std::uint8_t> tailOrBeginOfTail;
	std::vector<std::uint8_t>::const_iterator lastBeginOfFrame = symbolsTail.cend();
	for (auto & iter = symbolsTail.cbegin(); iter != symbolsTail.cend(); ++iter) {
		if (':' == *iter) {
			lastBeginOfFrame = iter;
		}
		// 0D 0A
		if (0x0A == *iter && (symbolsTail.cend() != lastBeginOfFrame)) {
			// check that previouse symbol was a 0x0D
			if (!tailOrBeginOfTail.empty() && 0x0D == tailOrBeginOfTail.back()) {
				tailOrBeginOfTail.push_back(*iter);
				std::vector<std::uint8_t> newTail(++iter, symbolsTail.cend());
				symbolsTail.swap(newTail);
				// cut off garbage from begin of the frame 
				std::vector<std::uint8_t> frame(lastBeginOfFrame, iter);
				return frame;
			}
			else {
				std::vector<std::uint8_t> newTail(++iter, symbolsTail.cend());
				symbolsTail.swap(newTail);
				// it's not a valid frame so clear it and return empty result without updatting of tailSymbols
				tailOrBeginOfTail.clear();
				return tailOrBeginOfTail;
			}
		}
		tailOrBeginOfTail.push_back(*iter);
	}
	// it's not a full valid frame so clear it and return empty result without updatting of tailSymbols
	tailOrBeginOfTail.clear();
	return tailOrBeginOfTail;
}

bool Device::isValidFrame(std::vector<std::uint8_t> symbols) {
	if (symbols.empty() || symbols.size() < 3)
		return false;

	if (*symbols.cbegin() != ':' || symbols.back() != 0x0A)
		return false;

	if (symbols[symbols.size() - 2] != 0x0D)
		return false;

	if (std::find(symbols.begin() + 1, symbols.end(), ':') != symbols.end())
		return false;

	return true;
}

bool Device::trimLeftSymbolsSequenceAsFrame(std::vector<std::uint8_t> & framePretenders) {
	if (framePretenders.empty())
		return false;

	auto & iter = std::find(framePretenders.cbegin(), framePretenders.cend(), ':');
	if (iter != framePretenders.cend()) {
		size_t index = std::distance(framePretenders.cbegin(), iter);
		std::vector<std::uint8_t> newTail(iter, framePretenders.cend());
		framePretenders.swap(newTail);
	}
	return false;
}

Device::Device(OVERLAPPED * const stateDialogOverlappedRD, DWORD * pMask, OVERLAPPED * const stateDialogOverlappedWR)
{
	overlappedRDPtr = stateDialogOverlappedRD;
	overlappedWRPtr = stateDialogOverlappedWR;
	overlappedWRPtr->hEvent = CreateEvent(NULL, true, false, NULL);
	overlappedWRPtr->Internal = 0;
	overlappedWRPtr->InternalHigh = 0;
	overlappedWRPtr->Offset = 0;
	overlappedWRPtr->OffsetHigh = 0;

	overlappedRDPtr->hEvent = CreateEvent(NULL, true, false, NULL);
	overlappedRDPtr->Internal = 0;
	overlappedRDPtr->InternalHigh = 0;
	overlappedRDPtr->Offset = 0;
	overlappedRDPtr->OffsetHigh = 0;
	mask = pMask;
}

bool Device::registerRectifier(RectifierInfo & info)
{
	if (registeredRectifiers.find(info.id) != registeredRectifiers.cend()) {
		return true;
	}
	LPCTSTR sPortName = info.comport;
	size_t openedCount = Device::openedPorts.count(sPortName);
	if (0 == openedCount) {
		hSerial = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, NULL/*FILE_FLAG_OVERLAPPED*/, 0);
		if (hSerial == INVALID_HANDLE_VALUE)
		{
			CString message;
			message.Format(L"Failed to open comport %s.", (LPCWSTR)info.comport);
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				message += L"Comport doesn't exists";
			}
			info.state = RectifierState::FAILED_TO_OPEN_COMPORT;
			AfxMessageBox(message, MB_YESNO | MB_ICONSTOP);
			//return;
		}
		else {
			info.hSerial = hSerial;
			Device::openedPorts[sPortName] = hSerial;
			Device::openedPortsCount[hSerial] = 1;
			registeredRectifiers[info.id] = info.id;
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
			//log += L"error setting serial port state\n";
			//m_CEditTestLog.SetWindowText(log);
			AfxMessageBox(L"got state error", MB_YESNO | MB_ICONSTOP);
			return false;
		}

		BOOL fSuccess = SetCommMask(hSerial, EV_RXCHAR | EV_TXEMPTY);

		if (!fSuccess)
		{
			// Handle the error. 
			CString message;
			message.Format(L"SetCommMask failed with error %u.\n", GetLastError());
			AfxMessageBox(message, MB_YESNO | MB_ICONSTOP);
			return false;
		}

		this->hSerial = hSerial;

		COMMTIMEOUTS commTimeouts;
		GetCommTimeouts(hSerial, &commTimeouts);
		commTimeouts.ReadIntervalTimeout = 10;
		commTimeouts.ReadTotalTimeoutConstant = 1000;
		commTimeouts.ReadTotalTimeoutMultiplier = 10;
		commTimeouts.WriteTotalTimeoutConstant = 1000;
		commTimeouts.WriteTotalTimeoutMultiplier = 10;

		SetCommTimeouts(hSerial, &commTimeouts);
	}
	else {
		// port already opened
		hSerial = Device::openedPorts[sPortName];
		info.hSerial = hSerial;
		openedPortsCount[hSerial] = openedPortsCount[hSerial] + 1;
		registeredRectifiers[info.id] = info.id;

	}
	info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
	info.state = RectifierState::SUCCESSFULLY_INITIALIZED;
	return true;
}

DWORD Device::WaitForReadSingleObject(DWORD timeout) {	//приостановить поток до прихода байта
	return WaitForSingleObject(overlappedRDPtr->hEvent, timeout);
}

BOOL Device::resetReadEvent() {
	return ResetEvent(overlappedRDPtr->hEvent);
}


Device::~Device()
{
	if (openedPortsCount[hSerial] > 1) {
		openedPortsCount[hSerial] = openedPortsCount[hSerial] - 1;
	}
	else {
		if (openedPortsCount[hSerial] > 0) {
			CloseHandle(overlappedWRPtr->hEvent);
			CloseHandle(overlappedRDPtr->hEvent);
			CloseHandle(hSerial);
			openedPortsCount[hSerial] = 0;
		}
	}
}

void Device::getFrameFromBuffer(std::vector<std::uint8_t> & rdSymbols) {
	rdSymbols.clear();
	if (!symbolsTail.empty()) {
		std::vector<std::uint8_t> beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
		rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
		if (isValidFrame(rdSymbols)) {
			return;
		}
		else {
			rdSymbols.clear();
		}
	}
}


void Device::getReadFrameFromPort(DWORD signal, std::vector<std::uint8_t> & rdSymbols) {
	DWORD temp, size;
	unsigned char bufrd[1024];
	rdSymbols.clear();

	if (signal != WAIT_OBJECT_0) {
		return;
	}


	BOOL res = WaitCommEvent(hSerial, mask, overlappedRDPtr);

	if (GetOverlappedResult(hSerial, overlappedRDPtr, &temp, true)) { //проверяем, успешно ли завершилась
																	//перекрываемая операция WaitCommEvent
		if ((*mask & EV_RXCHAR) != 0)				//если произошло именно событие прихода байта
		{
			//ClearCommError(hSerial, &temp, &comstat);	//нужно заполнить структуру COMSTAT
			//DWORD btr = comstat.cbInQue;                //и получить из неё количество принятых байтов
			//if (btr)                         			//если действительно есть байты для чтения
			//{
			BOOL wasRead = false;
			Sleep(1);
			do {
				BOOL innerRes = ReadFile(hSerial, bufrd, 1, &size, overlappedRDPtr);     //прочитать байты из порта в буфер программы
				if (size > 0) {   // если что-то принято, выводим
					for (DWORD i = 0; i < size; ++i) {
						symbolsTail.push_back(bufrd[i]);
					}
				}
			} while (size > 0);
			//}
		}
		if ((*mask & EV_TXEMPTY) != 0)
		{
			ResetEvent(overlappedRDPtr->hEvent);
			//ClearCommError(hSerial, &temp, &comstat);
		}
	}


	if (!symbolsTail.empty()) {
		//Device::trimLeftSymbolsSequenceAsFrame(rdSymbolsFrame);
		std::vector<std::uint8_t> beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
		rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
		while (!rdSymbols.empty() && !isValidFrame(rdSymbols)) {
			beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
			rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
		}
	}

	if (!Device::isValidFrame(rdSymbols)) {
		// wait for remain symbols
		if (!rdSymbols.empty()) {
			// append read symbols into the tail
			Device::trimLeftSymbolsSequenceAsFrame(rdSymbols);
			symbolsTail.insert(symbolsTail.cend(), rdSymbols.cbegin(), rdSymbols.cend());
		}
	}

}

void Device::sendCommand(HANDLE hSerial, std::vector<uint8_t> & frameSymbols, DWORD & dwBytesWritten, CString &log) {
	Sleep(50);
	const uint8_t * data = frameSymbols.data();
	DWORD dwSize = static_cast<DWORD>(frameSymbols.size());

	BOOL iRet = WriteFile(hSerial, data, dwSize, &dwBytesWritten, NULL);
	std::wstringstream ss;
	ss << dwSize << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
	std::wstring str1;
	str1 = ss.str();
	log += str1.c_str();

}


void Device::sendReplyData(std::vector<uint8_t> frameSymbols, DWORD & dwBytesWritten, CString &log) {

	const uint8_t * data = frameSymbols.data();
	DWORD dwSize = static_cast<DWORD>(frameSymbols.size());   // размер этой строки

	BOOL iRet = WriteFile(hSerial, data, dwSize, &dwBytesWritten, overlappedWRPtr);
	DWORD signal = WaitForSingleObject(overlappedWRPtr->hEvent, 1000);	//приостановить поток, пока не завершится
																				//перекрываемая операция WriteFile
																				//если операция завершилась успешно, установить соответствующий флажок
	std::wstringstream ss;
	if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hSerial, overlappedWRPtr, &dwBytesWritten, true))) {
		ss << L"sent - OK \n/n" << std::endl;
	}
	else {
		ss << L"Failed to send cm..." << std::endl;
	}

	ss << dwSize << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
	std::wstring str1;
	str1 = ss.str();
	log += str1.c_str();
}

RectifierState Device::readFromPort(std::vector<std::uint8_t> & rdSymbols, std::vector<uint8_t> & sendSymbols, DWORD & dwBytesWritten, CString &log) {
	rdSymbols.clear();
	unsigned char bufrd[1024];
	DWORD numberOfBytesTransfered;
	BOOL res = ReadFile(hSerial, bufrd, 1024, NULL, overlappedRDPtr);
	DWORD lastError = GetLastError();
	if (res || (!res && (lastError == ERROR_IO_PENDING))) {
		sendCommand(hSerial, sendSymbols, dwBytesWritten, log);
		BOOL res1 = GetOverlappedResult(hSerial, overlappedRDPtr, &numberOfBytesTransfered, true);
		lastError = GetLastError();
		if (numberOfBytesTransfered > 0) {   // если что-то принято, выводим
			for (DWORD i = 0; i < numberOfBytesTransfered; ++i) {
				symbolsTail.push_back(bufrd[i]);
			}
			DWORD numberOfBytesRead;
			BOOL res2 = ReadFile(hSerial, bufrd, 1024, &numberOfBytesRead, overlappedRDPtr);
			while (numberOfBytesRead > 0) {
				for (DWORD i = 0; i < numberOfBytesRead; ++i) {
					symbolsTail.push_back(bufrd[i]);
				}
				res2 = ReadFile(hSerial, bufrd, 1024, &numberOfBytesRead, overlappedRDPtr);
			}
		}
		else {
			return RectifierState::UNKNOWN_ERROR;
		}

		if (!symbolsTail.empty()) {
			//Device::trimLeftSymbolsSequenceAsFrame(rdSymbolsFrame);
			std::vector<std::uint8_t> beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
			rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
			while (!symbolsTail.empty() && !isValidFrame(rdSymbols)) {
				beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
				if (beginOfOrWholeFrame.empty()) {
					// cann't extract from tail anything
					break;
				}
				rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
			}
		}

		if (!Device::isValidFrame(rdSymbols)) {
			// wait for remain symbols
			if (!rdSymbols.empty()) {
				// append read symbols into the tail
				Device::trimLeftSymbolsSequenceAsFrame(rdSymbols);
				symbolsTail.insert(symbolsTail.cend(), rdSymbols.cbegin(), rdSymbols.cend());
			}
		}
		return RectifierState::OK;
	}
	//else if (lastError = ERROR_INVALID_USER_BUFFER) {
	//	return RectifierState::INVALID_USER_BUFFER;
	//}
	//else {
	//	return RectifierState::FAILED_TO_GET_STATE_F07;
	//}
	return RectifierState::UNKNOWN_ERROR;
}

RectifierState Device::readFromPort(std::vector<std::uint8_t> & rdSymbols) {
	rdSymbols.clear();
	unsigned char bufrd[1024];
	DWORD numberOfBytesTransfered;
	BOOL res = ReadFile(hSerial, bufrd, 1024, NULL, overlappedRDPtr);
	DWORD lastError = GetLastError();
	if (res || (!res && (lastError == ERROR_IO_PENDING))) {
		GetOverlappedResult(hSerial, overlappedRDPtr, &numberOfBytesTransfered, true);
		lastError = GetLastError();
		if (numberOfBytesTransfered > 0) {   // если что-то принято, выводим
			for (DWORD i = 0; i < numberOfBytesTransfered; ++i) {
				symbolsTail.push_back(bufrd[i]);
			}
		}
		else {
			return RectifierState::UNKNOWN_ERROR;
		}

		if (!symbolsTail.empty()) {
			//Device::trimLeftSymbolsSequenceAsFrame(rdSymbolsFrame);
			std::vector<std::uint8_t> beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
			rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
			while (!symbolsTail.empty() && !isValidFrame(rdSymbols)) {
				beginOfOrWholeFrame = getFrameFromTail(symbolsTail);
				if (beginOfOrWholeFrame.empty()) {
					// cann't extract from tail anything
					break;
				}
				rdSymbols.insert(rdSymbols.end(), beginOfOrWholeFrame.cbegin(), beginOfOrWholeFrame.cend());
			}
		}

		if (!Device::isValidFrame(rdSymbols)) {
			// wait for remain symbols
			if (!rdSymbols.empty()) {
				// append read symbols into the tail
				Device::trimLeftSymbolsSequenceAsFrame(rdSymbols);
				symbolsTail.insert(symbolsTail.cend(), rdSymbols.cbegin(), rdSymbols.cend());
			}
		}
		return RectifierState::OK;
	}
	//else if (lastError = ERROR_INVALID_USER_BUFFER) {
	//	return RectifierState::INVALID_USER_BUFFER;
	//}
	//else {
	//	return RectifierState::FAILED_TO_GET_STATE_F07;
	//}
	return RectifierState::UNKNOWN_ERROR;
}

bool checkForError(RectifierInfo & info) {
	if (info.getCommunicationState() == DeviceCommunicationState::FRAME_SENT) {
		info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	if (info.getCommunicationState() == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
		info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}
	return false;
}

void waiteForReply(RectifierInfo & info) {
	int count = 0;
	while (info.getCommunicationState() == DeviceCommunicationState::FRAME_SENT) {
		Sleep(++count);
		if (count > 200) {
			break;
		}
	}
}

void waiteForClearBufferReply(RectifierInfo & info) {
	int count = 0;
	while (info.getCommunicationState() != DeviceCommunicationState::CLEAR_BUFFER_SUCCEEDED) {
		Sleep(++count);
		if (count > 200) {
			break;
		}
	}
}

void doLog(DWORD dwBytesWritten, std::vector<uint8_t> & rdSymbols, CString & log) {
	// logging
	std::wstringstream ss;
	ss << dwBytesWritten << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
	std::wstring str1;
	str1 = ss.str();
	log += str1.c_str();

	ss.clear();
	ss << L"Started reading replay..." << std::endl;
	str1 = ss.str();
	for (auto symbol : rdSymbols) {
		ss << std::hex << symbol;
	}
	ss << std::endl;
	str1 = ss.str();
	log += str1.c_str();
}

bool Device::getRectifierState(RectifierInfo & info, bool plusTime) {
	DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
	CString log;

	//// clear buffer
	//// L"Send command 0x01";
	//std::vector<uint8_t> frameBytes1 = DeviceCommand::createCmdFrame(
	//	info.address, 0x43, DeviceCommand::GIVE_PREPARED_DATA_01);
	//std::vector<uint8_t> frameSymbols1 = DeviceCommand::convertToASCIIFrame(frameBytes1);
	//info.clearReceivedBuffer();
	//Device::sendCommand(hSerial, frameSymbols1, dwBytesWritten, log);
	//Sleep(10);
	//if (dwBytesWritten > 0) {
	//	info.setCommunicationState(DeviceCommunicationState::CLEAR_BUFFER);
	//	waiteForClearBufferReply(info);
	//	if (info.getCommunicationState() != DeviceCommunicationState::CLEAR_BUFFER_SUCCEEDED) {
	//		info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
	//		info.state = RectifierState::FAILED_TO_GET_STATE;
	//		return true;
	//	}
	//}

	std::vector<std::uint8_t> rdSymbols;
	symbolsTail.clear();
	//getFrameFromBuffer(rdSymbols);
	// L"Send command 0x05";
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
		info.address, 0x43, plusTime ? GET_DEVICE_STATE_05 : GET_CONCISE_DEVICE_STATE_07);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	Sleep(10);
	if (dwBytesWritten > 0) {
		info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
	}
	Sleep(10);

	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (should be read in separate  thread)
	waiteForReply(info);

	if (info.getCommunicationState() == DeviceCommunicationState::FRAME_SENT) {
		info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	if (info.getCommunicationState() == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
		info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		rdSymbols = info.getReceivedDataBuffer();
		
		// check the whole frame received
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState(DeviceCommunicationState::FRAME_SENT);
			Sleep(10);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}
			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
					info.state = RectifierState::FAILED_TO_GET_STATE;
					return true;
				}
			}
			else {
				info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
				info.state = RectifierState::FAILED_TO_GET_STATE;
				return true;
			}
		}
		else {
			// OK
		}

	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	doLog(dwBytesWritten, rdSymbols, log);
	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return true;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			return true;
		}
		// L"Send command 0x01";
		frameBytes = DeviceCommand::createCmdFrame(
			info.address, 0x43, DeviceCommand::GIVE_PREPARED_DATA_01);
		frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
		info.clearReceivedBuffer();
		Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
		Sleep(100);
		if (dwBytesWritten > 0) {
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
		}
		Sleep(1);

		// wait for reply (should be read in separate  thread)
		waiteForReply(info);

		if (info.getCommunicationState() == DeviceCommunicationState::FRAME_SENT) {
			info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return true;
		}

		if (info.getCommunicationState() == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return true;
		}

		if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
			// check the whole frame received
			rdSymbols = info.getReceivedDataBuffer();
			if (!isValidFrame(rdSymbols)) {
				// try read again
				info.setCommunicationState(DeviceCommunicationState::FRAME_SENT);
				Sleep(10);
				waiteForReply(info);
				if (checkForError(info)) {
					return true;
				}
				if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
					rdSymbols = info.getReceivedDataBuffer();
					if (!Device::isValidFrame(rdSymbols)) {
						info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
						info.state = RectifierState::FAILED_TO_GET_STATE;
						return true;
					}
				}
				else {
					info.setCommunicationState(DeviceCommunicationState::INIT_STATE);
					info.state = RectifierState::FAILED_TO_GET_STATE;
					return true;
				}
			}
			doLog(dwBytesWritten, rdSymbols, log);
			DeviceCommand::REPLY_DATA data;
			DeviceCommand::parseResponseFrame(rdSymbols, modbus_addr, modbus_func, data);
			if (data.data.size() > 0 && info.address == modbus_addr) {
				if (plusTime) {
					DeviceCommand::StateF05 state;
					if (DeviceCommand::parseDataForF05(data.data, state)) {
						info.state = RectifierState::FAILED_TO_PARSE_STATE;
						return true;
					}
					info.stateF05 = state;
				}
				else {
					DeviceCommand::StateF07 state;
					if (DeviceCommand::parseDataForF07(data.data, state)) {
						info.state = RectifierState::FAILED_TO_PARSE_STATE;
						return true;
					}
					else {
						info.stateF05.setHiA(state.aHi);
						info.stateF05.setLowA(state.aLow);
						info.stateF05.setV(state.V);
					}
				}
				info.state = RectifierState::OK;
			}
			else {
				if (info.address != modbus_addr) {
					info.state = RectifierState::DATA_ADDRESS_DOESNT_MATCH;
					return true;
				}
				else {
					info.state = RectifierState::FAILED_TO_GET_STATE;
					return true;
				}
			}
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}
	return false;
}

void setFrameIsSentFlag(RectifierInfo & info, DWORD dwBytesWritten) {
	Sleep(10);
	if (dwBytesWritten > 0) {
		info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
	}
	Sleep(10);
}



bool setRemoteControl(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> & rdSymbols, CString & log) {
	rdSymbols.clear();
	// set control to remote pult
	DWORD dwBytesWritten;
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
		info.address, 0x43, ACTIVATE_REMOTE_CONTROL_PANEL_06_01);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	setFrameIsSentFlag(info, dwBytesWritten);

	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (while is being read in separate thread)
	waiteForReply(info);

	// check for error
	if (checkForError(info)) {
		return true;
	}
	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols = info.getReceivedDataBuffer();
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}

			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				// check the whole frame received (if not - return an error)
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
					info.state = RectifierState::FAILED_TO_GET_STATE;
					// failed to send command 
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
				info.state = RectifierState::FAILED_TO_GET_STATE;
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			// OK
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	doLog(dwBytesWritten, rdSymbols, log);

	// check for reply is OK (code == 5)
	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}
	// OK
	return false;
}

std::vector<uint8_t> createVoltageData(
	uint8_t cmd, short current, short voltage, uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t param) {
	std::vector<uint8_t> data(8);
	data[0] = cmd;
	data[1] = (uint8_t)(current & 0xff);
	data[2] = (uint8_t)((current & 0xff00) >> 8);
	data[3] = (uint8_t)voltage;
	data[4] = hours;
	data[5] = minutes;
	data[6] = seconds;
	data[7] = param;
	return data;
}

std::vector<uint8_t> createProgramStepData(short address, std::vector<uint8_t> data) {
	std::vector<uint8_t> programStep(3);
	programStep[0] = address & 0xff;
	programStep[1] = (address & 0xff00) >> 8;
	uint8_t length = (uint8_t)data.size();
	programStep[2] = length;
	for (const auto & byte : data) {
		programStep.push_back(byte);
	}
	return programStep;
}

bool writeVoltageProgram(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> & rdSymbols, CString & log) {
	rdSymbols.clear();
	// for now only first program step is set, second step is null
	const std::vector<uint8_t> zeroStep(8);
	// set voltage and current
	DWORD dwBytesWritten;
	short voltage = (short)round(info.cmdToSend.voltage * 10);
	short current = (short)round(info.cmdToSend.current);
	std::vector<uint8_t> voltageData = createVoltageData(0x83, current, voltage, 10, 0, 0, 0);
	for (const auto & b : zeroStep) {
		voltageData.push_back(b);
	}
	std::vector<uint8_t> programStepData = createProgramStepData(0, voltageData);

	DeviceCommand::DATA SET_VOLTAGE_PROGRAM_STEP_03_01 = { (uint8_t)(programStepData[2] + (uint8_t)0x05)/*size*/,(uint8_t)0x03/*addr*/,(uint8_t)0x01/*cmd*/, programStepData };
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, SET_VOLTAGE_PROGRAM_STEP_03_01);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	setFrameIsSentFlag(info, dwBytesWritten);
	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (while is being read in separate thread)
	waiteForReply(info);

	// check for error
	if (checkForError(info)) {
		return true;
	}
	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols = info.getReceivedDataBuffer();
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}

			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				// check the whole frame received (if not - return an error)
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
					info.state = RectifierState::FAILED_TO_GET_STATE;
					// failed to send command 
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
				info.state = RectifierState::FAILED_TO_GET_STATE;
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			// OK
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}

	doLog(dwBytesWritten, rdSymbols, log);

	// check for reply is OK (code == 5)
	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_SET_VOLTAGE;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_SET_VOLTAGE;
		return true;
	}
	info.cmdToSend.status = CmdStatus::SUCCEEDED;
	return false;
}

bool sendFrame(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> frameBytes,
	std::vector<uint8_t> & rdSymbols, CString & log, DWORD & dwBytesWritten, bool checkForReply = true) {
	rdSymbols.clear();
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	setFrameIsSentFlag(info, dwBytesWritten);
	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (while is being read in separate thread)
	waiteForReply(info);

	// check for error
	if (checkForError(info)) {
		return true;
	}
	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols = info.getReceivedDataBuffer();
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}

			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				// check the whole frame received (if not - return an error)
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
					// failed to send command 
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			// OK
		}
	}
	else {
		return true;
	}

	doLog(dwBytesWritten, rdSymbols, log);

	if (checkForReply) {
		// check for reply is OK (code == 5)
		if (!rdSymbols.empty()) {
			DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
			if (reply_code != 5) {
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			if (modbus_addr != info.address) {
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			return true;
		}
	}
	return false;
}
bool Device::readProgram(RectifierInfo & info) {
	CString log;
	std::vector<uint8_t> rdSymbols;
	// read only first program step (assume second step is null)
	std::vector<uint8_t> programFirstStepAddrSize = { 0/*addr low byte*/, 0/*addr hi byte*/, 8/*bytes to read*/ };
	DeviceCommand::DATA READ_FIRST_PROGRAM_STEP_03_00 =
	{ (uint8_t)0x05/*size*/,(uint8_t)0x03/*addr*/,(uint8_t)0x00/*cmd*/, programFirstStepAddrSize };
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, READ_FIRST_PROGRAM_STEP_03_00);
	DWORD dwBytesWritten;
	if (sendFrame(info.hSerial, info, frameBytes, rdSymbols, log, dwBytesWritten)) {
		info.state = RectifierState::FAILED_TO_READ_PROGRAM;
		return true;
	}

	// L"Send command 0x01";
	frameBytes = DeviceCommand::createCmdFrame(
		info.address, 0x43, DeviceCommand::GIVE_PREPARED_DATA_01);
	if (sendFrame(info.hSerial, info, frameBytes, rdSymbols, log, dwBytesWritten, false)) {
		info.state = RectifierState::FAILED_TO_GET_DELAYED_REPLY;
		return true;
	}

	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	DeviceCommand::REPLY_DATA data;
	// parse program data
	DeviceCommand::parseResponseFrame(rdSymbols, modbus_addr, modbus_func, data);
	if (data.data.size() > 0 && info.address == modbus_addr) {
		info.memory = data.data;
	}
	return false;
}


bool setStartProgram(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> & rdSymbols, CString & log) {
	rdSymbols.clear();
	DWORD dwBytesWritten;
	DeviceCommand::DATA START_PROGRAM_FROM_FIRST_STEP_06_03 = { 0x03/*size*/,0x06/*addr*/,0x03/*cmd*/, {0x00} };
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, START_PROGRAM_FROM_FIRST_STEP_06_03);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	setFrameIsSentFlag(info, dwBytesWritten);
	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (while is being read in separate thread)
	waiteForReply(info);

	// check for error
	if (checkForError(info)) {
		return true;
	}
	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols = info.getReceivedDataBuffer();
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}

			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				// check the whole frame received (if not - return an error)
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
					info.state = RectifierState::FAILED_TO_GET_STATE;
					// failed to send command 
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
				info.state = RectifierState::FAILED_TO_GET_STATE;
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			// OK
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return true;
	}
	// logging
	doLog(dwBytesWritten, rdSymbols, log);

	// check for reply is OK (code == 5)
	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_SET_VOLTAGE;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_SET_VOLTAGE;
		return true;
	}
	info.cmdToSend.status = CmdStatus::SUCCEEDED;
	return false;
}


bool stopExecutingProgram(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> & rdSymbols, CString & log) {
	rdSymbols.clear();
	DWORD dwBytesWritten;
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, STOP_EXECUTING_PROGRAMM_06_09);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.clearReceivedBuffer();
	Device::sendCommand(hSerial, frameSymbols, dwBytesWritten, log);
	setFrameIsSentFlag(info, dwBytesWritten);
	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (while is being read in separate thread)
	waiteForReply(info);

	// check for error
	if (checkForError(info)) {
		return true;
	}
	if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols = info.getReceivedDataBuffer();
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.setCommunicationState( DeviceCommunicationState::FRAME_SENT);
			waiteForReply(info);
			if (checkForError(info)) {
				return true;
			}

			if (info.getCommunicationState() == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols = info.getReceivedDataBuffer();
				// check the whole frame received (if not - return an error)
				if (!Device::isValidFrame(rdSymbols)) {
					info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
					info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
					// failed to send command 
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				info.setCommunicationState( DeviceCommunicationState::INIT_STATE);
				info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
		}
		else {
			// OK
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
		return true;
	}
	// logging
	doLog(dwBytesWritten, rdSymbols, log);

	// check for reply is OK (code == 5)
	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			// failed to send command 
			info.cmdToSend.status = CmdStatus::FAILED;
			return true;
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
		return true;
	}
	info.cmdToSend.status = CmdStatus::SUCCEEDED;
	return false;
}

bool testDevice(HANDLE hSerial, RectifierInfo & info, std::vector<uint8_t> & rdSymbols, CString & log) {
	rdSymbols.clear();
	DWORD dwBytesWritten;
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, TEST_POWER_MODULES_02_82);
	if (sendFrame(hSerial, info, frameBytes, rdSymbols, log, dwBytesWritten)) {
		info.state = RectifierState::FAILED_TO_TEST_POWER_MODUES;
		return true;
	}
	// wait 3 seconds
	Sleep(3000);
	rdSymbols.clear();
	frameBytes = DeviceCommand::createCmdFrame(info.address, 0x43, SWITCH_OFF_POWER_MODULE_02_80);
	if (sendFrame(hSerial, info, frameBytes, rdSymbols, log, dwBytesWritten)) {
		info.state = RectifierState::FAILED_TO_TEST_POWER_MODUES;
		return true;
	}

	info.cmdToSend.status = CmdStatus::SUCCEEDED;
	return false;
}

bool Device::executeCmd(RectifierInfo & info) {
	if (info.cmdToSend.status == CmdStatus::EMPTY
		|| info.cmdToSend.status == CmdStatus::SUCCEEDED
		|| info.cmdToSend.status == CmdStatus::FAILED) {
		return false;
	}

	//	DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
	CString log;

	// clear buffer
	std::vector<std::uint8_t> rdSymbols;
	symbolsTail.clear();
	switch (info.cmdToSend.rectifierCmd) {
	case RectifierCmd::SET_VOLTAGE_AND_CURRENT:
		switch (info.cmdToSend.status) {
		case CmdStatus::PREPARED:
			// send command to set remote control
			if (setRemoteControl(hSerial, info, rdSymbols, log)) {
				// some error occured
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			if (getRectifierState(info, true)) {
				//some error occurred
				info.state = RectifierState::FAILED_TO_GET_STATE;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			// check that remote control is activated
			if ((info.stateF05.getControlByte() & 0x03) != 0x03) {
				info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}

			if ((info.stateF05.getControlByte() & 0x04) == 0x04) {
				// stop executing program 
				if (stopExecutingProgram(hSerial, info, rdSymbols, log)) {
					info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				// send command to set remote control
				if (setRemoteControl(hSerial, info, rdSymbols, log)) {
					// some error occured
					// failed to send command
					info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				if (getRectifierState(info, true)) {
					//some error occurred
					info.state = RectifierState::FAILED_TO_GET_STATE;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				// check that remote control is activated
				if ((info.stateF05.getControlByte() & 0x03) != 0x03) {
					info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				// test device
				if (testDevice(hSerial, info, rdSymbols, log)) {
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			// set voltage and current
			if (writeVoltageProgram(hSerial, info, rdSymbols, log)) {
				info.state = RectifierState::FAILED_TO_SET_VOLTAGE;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			// read program from
			if (readProgram(info)) {
				info.state = RectifierState::FAILED_TO_START_EXECUTING_PROGRAMM;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			break;
		}
		return false;

	case RectifierCmd::START_EXISTIN_PROGRAM:
		switch (info.cmdToSend.status) {
		case CmdStatus::PREPARED:
			// send command to set remote control
			if (setRemoteControl(hSerial, info, rdSymbols, log)) {
				// some error occured
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			if (getRectifierState(info, true)) {
				//some error occurred
				info.state = RectifierState::FAILED_TO_GET_STATE;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			// check that remote control is activated
			if ((info.stateF05.getControlByte() & 0x03) != 0x03) {
				info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}

			if ((info.stateF05.getControlByte() & 0x04) == 0x04) {
				// stop executing program 
				if (stopExecutingProgram(hSerial, info, rdSymbols, log)) {
					info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				// send command to set remote control
				if (setRemoteControl(hSerial, info, rdSymbols, log)) {
					// some error occured
					// failed to send command
					info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				if (getRectifierState(info, true)) {
					//some error occurred
					info.state = RectifierState::FAILED_TO_GET_STATE;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
				// check that remote control is activated
				if ((info.stateF05.getControlByte() & 0x03) != 0x03) {
					info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			else {
				// test device
				if (testDevice(hSerial, info, rdSymbols, log)) {
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			// start program from first step
			if (setStartProgram(hSerial, info, rdSymbols, log)) {
				info.state = RectifierState::FAILED_TO_START_EXECUTING_PROGRAMM;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			break;
		}
		return false;
	case RectifierCmd::STOP_EXISTIN_PROGRAM:
		switch (info.cmdToSend.status) {
		case CmdStatus::PREPARED:
			// send command to set remote control
			if (setRemoteControl(hSerial, info, rdSymbols, log)) {
				// some error occured
				// failed to send command 
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			if (getRectifierState(info, true)) {
				//some error occurred
				info.state = RectifierState::FAILED_TO_GET_STATE;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}
			// check that remote control is activated
			if ((info.stateF05.getControlByte() & 0x03) != 0x03) {
				info.state = RectifierState::FAILED_TO_SET_REMOTE_CONTROL;
				info.cmdToSend.status = CmdStatus::FAILED;
				return true;
			}

			if ((info.stateF05.getControlByte() & 0x04) == 0x04) {
				// stop executing program 
				if (stopExecutingProgram(hSerial, info, rdSymbols, log)) {
					info.state = RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM;
					info.cmdToSend.status = CmdStatus::FAILED;
					return true;
				}
			}
			break;
		}
		return false;
	}
	return false;
}

void Device::clearReciveBuffer()
{
	uint8_t buffer[1024];
	DWORD size;
	do {
		BOOL res = ReadFile(this->hSerial, buffer, 1024, &size, overlappedRDPtr);
	} while (size > 0);

}

void log_msg(std::wofstream * log, int loglevel, CString str) {
	CString t = CTime::GetCurrentTime().Format("%H:%M:%S");
	(*log) << t.GetString() << '\n';
	(*log) << str.GetString() << '\n';
	log->flush();
}

UINT ThreadProc(LPVOID par) {
	SThread_param * param;
	param = (SThread_param *)par;
	param->statePtr[0] = 1;//выставили флаг, что поток работы с ком портом запущен
	int logLevel = param->logLevel;

	//FILE * mainLog;
	//_wfopen_s(&mainLog, "mainlog", L"wt");
	//mainLog->
	TCHAR	szBuffer[256];
	UINT    nActual = 0;
	CFile	myFile;

	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		DWORD error = GetLastError();
	}

	const fs::path app_dir(fs::path(szPath).parent_path());

	CString t = CTime::GetCurrentTime().Format("%m%d%Y_%H_%M");
	
	CString name(app_dir.c_str());
	name += L"/main_thread";
	name += t;
	name += L".log";
	//CStdioFile file;
	//CFileException pError;
	//if (file.Open(name.GetString(), CFile::modeCreate | CFile::modeWrite | CFile::typeText), &pError)
	//{
	//	file.WriteString(L"Start main thread");
	//}

	//CString Placering = "C:\\ZigBeeLogs\\Log.txt";
	std::wofstream Logfil;

	Logfil.open(name.GetString(), std::ios::app);
	bool res = Logfil.fail();
	log_msg(&Logfil, logLevel, L"start main thread");

	//if (myFile.Open(name.GetString(), CFile::modeCreate |
	//	CFile::modeNoTruncate | CFile::typeUnicode))
	//{
	//	myFile.Write(szBuffer, sizeof(szBuffer));
	//	myFile.Flush();
	//}

	int cnt = 0;
	std::wstringstream ss;
	ss << ++cnt;

	Device device(param->mainOverlappedRD, param->pMask, param->mainOverlappedWR);

	while (1) {
		if (param->statePtr[0] == 2) {
			//команда на выход из потока работы с ком портом
			break;
		}

		for (auto & rectInfo : param->m_rectifierConfigs[0]) {
			// lock info in the thread
			RectifierInfo & info = rectInfo.second;
	
			// for now only one port for all rectifiers
			if (info.state != RectifierState::OK
				&& info.state != RectifierState::NOT_INITIALIZED
				) {
				if (++info.badStateSkipCount > 0 && info.badStateSkipCount < 100) {
					continue;
				}
				else {
					info.badStateSkipCount = 0;
				}
			}
			else {
				info.badStateSkipCount = 0;
			}
			if (RectifierState::NOT_INITIALIZED == info.state) {
				device.registerRectifier(info);
				log_msg(&Logfil, logLevel, L"register rectifier");
			}
			if (info.memory.size() < 7) {
				//read memory
				device.readProgram(info);
				device.registerRectifier(info);
				log_msg(&Logfil, logLevel, L"read memory");
			}
			//device.clearReciveBuffer();
			if (info.cmdToSend.status == CmdStatus::PREPARED) {
				CString msg = L"start executing cmd";
				msg.Format(L"start executing cmd (%s)", info.cmdToSend.rectifierCmd);
				log_msg(&Logfil, logLevel, msg);
				device.executeCmd(info);
				log_msg(&Logfil, logLevel, L"cmd executed");
			}
			log_msg(&Logfil, logLevel, L"get state");
			device.getRectifierState(info, true);
			CSingleLock lock(&info.cs, true);
			// info.recivedData.status = ++cnt;
			lock.Unlock();
			PostMessage((HWND)param->wnd, WM_COMMAND, 7, 7);
			Sleep(500);
		}

	}
	param->statePtr[0] = 3;
	AfxEndThread(0);
	Logfil.close();
	return 0;
}

UINT ReadingComPortThread(LPVOID par) {
	Reading_thread_param * param = (Reading_thread_param *)par;
	param->readingThreadState[0] = 1; // thread reading port started

	int logLevel = param->logLevel;

	//FILE * mainLog;
	//_wfopen_s(&mainLog, "mainlog", L"wt");
	//mainLog->
	TCHAR	szBuffer[256];
	UINT    nActual = 0;
	CFile	myFile;

	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		DWORD error = GetLastError();
	}

	const fs::path app_dir(fs::path(szPath).parent_path());

	CString t = CTime::GetCurrentTime().Format("%m%d%Y_%H_%M");

	CString name(app_dir.c_str());
	name += L"/reading_thread";
	name += t;
	name += L".log";
	std::wofstream readThreadLog;
	readThreadLog.open(name.GetString(), std::ios::app);
	bool res = readThreadLog.fail();

	int cnt = 0;
	std::wstringstream ss;
	ss << ++cnt;
	OVERLAPPED overlappedRD;
	overlappedRD.hEvent = CreateEvent(NULL, true, false, NULL);
	overlappedRD.Internal = 0;
	overlappedRD.InternalHigh = 0;
	overlappedRD.Offset = 0;
	overlappedRD.OffsetHigh = 0;
	//	DWORD mask;


	while (1) {
		if (param->readingThreadState[0] == 2) {
			//команда на выход из потока работы с ком портом
			break;
		}
		// for now only one port for all rectifiers
		for (auto & rectInfo : param->m_rectifierConfigs[0]) {
			RectifierInfo & info = rectInfo.second;
			DeviceCommunicationState commState = info.getCommunicationState();
			if (commState == DeviceCommunicationState::FRAME_SENT) {
				// command sended try to read reply
				log_msg(&readThreadLog, logLevel, L"try read reply");
				std::vector<std::uint8_t> symbols;
				//				DWORD trans;
				DWORD rdBytes;
				PortState portState = PortState::PORT_UNKNOWN;
				unsigned char bufrd[1024];
				BOOL res1 = ReadFile(info.hSerial, bufrd, 1024, &rdBytes, NULL);
				if (res1) {
					if (rdBytes > 0) {   // если что-то принято, выводим
						for (DWORD i = 0; i < rdBytes; ++i) {
							symbols.push_back(bufrd[i]);

						}
						log_msg(&readThreadLog, logLevel, L"port read succeeded");
						portState = PortState::READ_SUCCEEDED;
					}
				}
				else {
					DWORD lastError = GetLastError();
					//					if (portState == PortState::READ_TIMEOUT) {
					info.setCommunicationState( DeviceCommunicationState::REPLY_READ_TIMEOUT);
					CString msg;
					msg.Format(L"read reply timeout (lastError = %d)", lastError);
					log_msg(&readThreadLog, logLevel, msg);
				}

				if (portState == PortState::READ_SUCCEEDED) {
					// put read bytes in buffer
					//info.recivedData.status += 1;
					info.addToReceivedBuffer(symbols);
					info.setCommunicationState( DeviceCommunicationState::READ_SUCCEEDED);
					log_msg(&readThreadLog, logLevel, L"read reply succeeded");
				}
				else if (portState == PortState::READ_TIMEOUT) {
					info.setCommunicationState( DeviceCommunicationState::REPLY_READ_TIMEOUT);
					log_msg(&readThreadLog, logLevel, L"read reply timeout");
				}
			}
			if (commState == DeviceCommunicationState::CLEAR_BUFFER) {
				// command sended try to read buffer
				log_msg(&readThreadLog, logLevel, L"clear buffer");
				DWORD rdBytes;
				PortState portState = PortState::PORT_UNKNOWN;
				unsigned char bufrd[1024];
				BOOL res1 = ReadFile(info.hSerial, bufrd, 1024, &rdBytes, NULL);
				info.clearReceivedBuffer();
				info.setCommunicationState(DeviceCommunicationState::CLEAR_BUFFER_SUCCEEDED);
			}
		}
		Sleep(50);
	}
	param->readingThreadState[0] = 3;
	AfxEndThread(0);
	return 0;
}

ComPort::ComPort()
{
	this->portHandle = nullptr;
}

ComPort::ComPort(HANDLE port)
{
	setPortHandle(port);
}

ComPort::~ComPort()
{
	portBlock.Lock();
	CloseHandle(portHandle);
	portBlock.Unlock();
}

BOOL ComPort::lock()
{
	return portBlock.Lock();
}

PortState Comport::readPort(HANDLE hSerial, OVERLAPPED * const overlappedRD, DWORD * pMask, std::vector<std::uint8_t>& rdSymbols)
{
	rdSymbols.clear();
	unsigned char bufrd[1024];
	DWORD numberOfBytesTransfered;
	BOOL res = ReadFile(hSerial, bufrd, 1024, NULL, overlappedRD);
	DWORD lastError = GetLastError();
	if (res || (!res && (lastError == ERROR_IO_PENDING))) {
		BOOL waitRes = GetOverlappedResult(hSerial, overlappedRD, &numberOfBytesTransfered, true);
		if (waitRes == 0) {
			lastError = GetLastError();
			return PortState::READ_TIMEOUT;
		}

		if (numberOfBytesTransfered > 0) {   // если что-то принято, выводим
			for (DWORD i = 0; i < numberOfBytesTransfered; ++i) {
				rdSymbols.push_back(bufrd[i]);
			}
			PortState::READ_SUCCEEDED;
		}
		else {
			return PortState::READ_EMPTY_BUFFER;
		}
	}
	return PortState::READ_ERROR;
}

bool Comport::isValidFrame(std::vector<std::uint8_t> & symbols) {
	if (symbols.empty() || symbols.size() < 3)
		return false;

	if (*symbols.cbegin() != ':' || symbols.back() != 0x0A)
		return false;

	if (symbols[symbols.size() - 2] != 0x0D)
		return false;

	if (std::find(symbols.begin() + 1, symbols.end(), ':') != symbols.end())
		return false;

	return true;
}

void RectifierInfo::setCommunicationState(DeviceCommunicationState commState) {
	// Multythreading Synchronization
	CSingleLock lock(&this->cs, true);
	this->communicationState = commState;
	lock.Unlock();
}

DeviceCommunicationState RectifierInfo::getCommunicationState()
{
	CSingleLock lock(&this->cs, true);
	DeviceCommunicationState st = this->communicationState;
	lock.Unlock();
	return st;
}

std::vector<std::uint8_t> RectifierInfo::getReceivedDataBuffer()
{
	
	std::vector<std::uint8_t> rdSymbols;
	CSingleLock lock(&this->cs, true);
	auto result = std::find(this->recivedData.buffer.rbegin(), this->recivedData.buffer.rend(), ':');
	if (result != this->recivedData.buffer.rend()) {
		size_t dist = std::distance(result, this->recivedData.buffer.rend());
		rdSymbols.insert(rdSymbols.begin(), this->recivedData.buffer.begin() + dist - 1, this->recivedData.buffer.end());
	}
	else {
		rdSymbols.insert(rdSymbols.begin(), this->recivedData.buffer.begin(), this->recivedData.buffer.end());
	}
	lock.Unlock();
	return rdSymbols;
}

void RectifierInfo::clearReceivedBuffer()
{
	CSingleLock lock(&this->cs, true);
	this->recivedData.buffer.clear();
	lock.Unlock();
}

void RectifierInfo::addToReceivedBuffer(std::vector<std::uint8_t> symbols)
{
	CSingleLock lock(&this->cs, true);
	this->recivedData.buffer.insert(
		this->recivedData.buffer.end(),
		symbols.begin(),
		symbols.end());
	lock.Unlock();
}
