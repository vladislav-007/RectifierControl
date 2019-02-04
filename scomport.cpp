#include "stdafx.h"
#include "scomport.h"
#include "RectifierCommand.h"
#include <vector>

//#include "cncload.h"

#include <Windows.h>
#include <sstream>

//OVERLAPPED overlappedWR;
//OVERLAPPED overlappedRD;

std::map<CString, HANDLE> Device::openedPorts;
std::map<HANDLE, int> Device::openedPortsCount;

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

bool Device::isValidFrame(std::vector<std::uint8_t> & symbols) {
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
			message.Format(L"Failed to open comport %s.", info.comport);
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
			message.Format(L"SetCommMask failed with error %d.\n", GetLastError());
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
	info.communcationState = DeviceCommunicationState::INIT_STATE;
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
				ReadFile(hSerial, bufrd, 1, &size, overlappedRDPtr);     //прочитать байты из порта в буфер программы
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

void Device::sendCommand(std::vector<uint8_t> & frameSymbols, DWORD & dwBytesWritten, CString &log) {
	const uint8_t * data = frameSymbols.data();
	DWORD dwSize = static_cast<DWORD>(frameSymbols.size());

	//BOOL iRet = WriteFile(hSerial, data, dwSize, NULL, overlappedWRPtr);
	BOOL iRet = WriteFile(hSerial, data, dwSize, &dwBytesWritten, NULL);
	//DWORD signal = WaitForSingleObject(overlappedWRPtr->hEvent, 1000);	//приостановить поток, пока не завершится
																	//перекрываемая операция WriteFile
																	//если операция завершилась успешно, установить соответствующий флажок
	std::wstringstream ss;
	//if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hSerial, overlappedWRPtr, &dwBytesWritten, true))) {
	//	ss << L"sent - OK \n/n" << std::endl;
	//}
	//else {
	//	ss << L"Failed to send cm..." << std::endl;
	//}

	ss << dwSize << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
	std::wstring str1;
	str1 = ss.str();
	log += str1.c_str();
	//Sleep(150);

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
		sendCommand(sendSymbols, dwBytesWritten, log);
		bool res1 = GetOverlappedResult(hSerial, overlappedRDPtr, &numberOfBytesTransfered, true);
		lastError = GetLastError();
		if (numberOfBytesTransfered > 0) {   // если что-то принято, выводим
			for (DWORD i = 0; i < numberOfBytesTransfered; ++i) {
				symbolsTail.push_back(bufrd[i]);
			}
			DWORD numberOfBytesRead;
			ReadFile(hSerial, bufrd, 1024, &numberOfBytesRead, overlappedRDPtr);
			while (numberOfBytesRead > 0 ) {
				for (DWORD i = 0; i < numberOfBytesRead; ++i) {
					symbolsTail.push_back(bufrd[i]);
				}
				ReadFile(hSerial, bufrd, 1024, &numberOfBytesRead, overlappedRDPtr);
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
	DWORD numberOfBytesTransfered, numberOfBytesWritten;
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


	////ожидать события приёма байта (это и есть перекрываемая операция)
	//if (WaitCommEvent(hSerial, mask, overlappedRDPtr)) {
	//	if ((*mask & EV_RXCHAR) != 0)				//если произошло именно событие прихода байта
	//	{
	//		DWORD temp, size;
	//		COMSTAT comstat;
	//		unsigned char bufrd[1024];
	//		ClearCommError(hSerial, &temp, &comstat);	//нужно заполнить структуру COMSTAT
	//		DWORD btr = comstat.cbInQue;                //и получить из неё количество принятых байтов
	//		if (btr)                         			//если действительно есть байты для чтения
	//		{
	//			ReadFile(hSerial, bufrd, btr, &size, overlappedRDPtr);     //прочитать байты из порта в буфер программы
	//			bool endOfFrameDetected = false;
	//			if (size > 0) {   // если что-то принято, выводим
	//				for (DWORD i = 0; i < size; ++i) {
	//					symbolsTail.push_back(bufrd[i]);
	//				}
	//			}
	//		}
	//	}
	//	if ((*mask & EV_TXEMPTY) != 0)
	//	{
	//		// To do.
	//		ResetEvent(overlappedRDPtr->hEvent);
	//	}
	//}
	//else
	//{
	//	DWORD dwRet = GetLastError();
	//	if (ERROR_IO_PENDING == dwRet)
	//	{
	//		printf("I/O is pending...\n");
	//		DWORD signal = WaitForReadSingleObject(5000); //приостановить поток до прихода байта
	//		if (signal == WAIT_OBJECT_0)				        //если событие прихода байта произошло
	//		{
	//			//resetReadEvent();
	//			// try to read port per one frame 
	//			getReadFrameFromPort(signal, rdSymbols);
	//			if (!Device::isValidFrame(rdSymbols)) {
	//				// wait for remain symbols
	//				if (!rdSymbols.empty()) {
	//					// append read symbols into the tail
	//					//trimLeftSymbolsSequenceAsFrame(rdSymbolsFrame);
	//					symbolsTail.insert(symbolsTail.cend(), rdSymbols.cbegin(), rdSymbols.cend());
	//				}
	//				Sleep(100);
	//				getReadFrameFromPort(signal, rdSymbols);
	//				if (!Device::isValidFrame(rdSymbols)) {
	//					if (!rdSymbols.empty()) {
	//						// append read symbols into the tail
	//						//trimLeftSymbolsSequenceAsFrame(rdSymbolsFrame);
	//						symbolsTail.insert(symbolsTail.cend(), rdSymbols.cbegin(), rdSymbols.cend());
	//					}
	//				}
	//			}
	//		}
	//	}
	//	// To do.
	//	else {
	//		printf("Wait failed with error %d.\n", GetLastError());
	//	}
	//}
//}


//void Device::getRectifierState(RectifierInfo & info) {
//	//HANDLE hSerial = info.hSerial;
//
//	DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
//	CString log;
//
//	// L"Send command 0x07";
//	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
//		info.address, 0x43, GET_CONCISE_DEVICE_STATE_07);
//	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
//	Device::sendCommand(frameSymbols, dwBytesWritten, log);
//	std::wstringstream ss;
//	ss << dwBytesWritten << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
//	std::wstring str1;
//	str1 = ss.str();
//	log += str1.c_str();
//	ss.clear();
//	ss << L"Started reading replay..." << std::endl;
//	std::vector<std::uint8_t> rdSymbols;
//	Sleep(1000);
//	getFrameFromBuffer(rdSymbols);
//	if (!isValidFrame(rdSymbols)) {
//		readFromPort(rdSymbols);
//	}
//	
//	str1 = ss.str();
//	for (auto symbol : rdSymbols) {
//		ss << std::hex << symbol;
//	}
//	ss << std::endl;
//	str1 = ss.str();
//	log += str1.c_str();
//
//	std::uint8_t modbus_addr;
//	std::uint8_t modbus_func;
//	std::uint8_t reply_code;
//
//	if (!rdSymbols.empty()) {
//		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
//		if (reply_code != 5) {
//			info.state = RectifierState::FAILED_TO_GET_STATE;
//			return;
//		}
//		// L"Send command 0x01";
//		frameBytes = DeviceCommand::createCmdFrame(
//			info.address, 0x43, DeviceCommand::GIVE_PREPARED_DATA_01);
//		frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
//		Device::sendCommand(frameSymbols, dwBytesWritten, log);
//		ss.clear();
//		ss << dwBytesWritten << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
//		str1 = ss.str();
//		log += str1.c_str();
//		getFrameFromBuffer(rdSymbols);
//		if (!isValidFrame(rdSymbols)) {
//			readFromPort(rdSymbols);
//		}
//		for (auto symbol : rdSymbols) {
//			ss << std::hex << symbol;
//		}
//		ss << std::endl;
//		str1 = ss.str();
//		log += str1.c_str();
//		DeviceCommand::REPLY_DATA data;
//		DeviceCommand::parseResponseFrame(rdSymbols, modbus_addr, modbus_func, data);
//		if (data.data.size() > 0) {
//			DeviceCommand::StateF07 state = DeviceCommand::parseDataForF07(data.data);
//			info.stateF05 = state;
//			info.state = RectifierState::OK;
//		}
//		else {
//			info.state = RectifierState::FAILED_TO_GET_STATE;
//		}
//	}
//	else {
//		info.state = RectifierState::FAILED_TO_GET_STATE;
//	}
//	
//	//CloseHandle(hSerial);
//}

void Device::getRectifierState(RectifierInfo & info, bool plusTime) {
	DWORD dwBytesWritten;    // тут будет количество собственно переданных байт
	CString log;

	// clear buffer
	std::vector<std::uint8_t> rdSymbols;
	symbolsTail.clear();
	//getFrameFromBuffer(rdSymbols);
	// L"Send command 0x07";
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
		info.address, 0x43, plusTime ? GET_DEVICE_STATE_05 : GET_CONCISE_DEVICE_STATE_07);
	std::vector<uint8_t> frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
	info.recivedData.buffer.clear();
	Device::sendCommand(frameSymbols, dwBytesWritten, log);
	Sleep(10);
	if (dwBytesWritten > 0) {
		info.communcationState = DeviceCommunicationState::FRAME_SENDED;
	}
	Sleep(10);

	std::uint8_t modbus_addr;
	std::uint8_t modbus_func;
	std::uint8_t reply_code;

	// wait for replay (should be read in separate  thread)
	int count = 0;
	while (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
		Sleep(++count);
		if (count > 100) {
			break;
		}
	}

	if (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
		info.communcationState = DeviceCommunicationState::INIT_STATE;
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return;
	}

	if (info.communcationState == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
		info.communcationState = DeviceCommunicationState::INIT_STATE;
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return;
	}

	if (info.communcationState == DeviceCommunicationState::READ_SUCCEEDED) {
		// check the whole frame received
		rdSymbols.insert(rdSymbols.begin(), info.recivedData.buffer.begin(), info.recivedData.buffer.end());
		if (!Device::isValidFrame(rdSymbols)) {
			// try read again
			info.communcationState = DeviceCommunicationState::FRAME_SENDED;
			Sleep(1);
			if (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
				info.communcationState = DeviceCommunicationState::INIT_STATE;
				info.state = RectifierState::FAILED_TO_GET_STATE;
				return;
			}

			if (info.communcationState == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
				info.communcationState = DeviceCommunicationState::INIT_STATE;
				info.state = RectifierState::FAILED_TO_GET_STATE;
				return;
			}
			if (info.communcationState == DeviceCommunicationState::READ_SUCCEEDED) {
				rdSymbols.insert(rdSymbols.end(), info.recivedData.buffer.begin(), info.recivedData.buffer.end());
				if (!Device::isValidFrame(rdSymbols)) {
					info.communcationState = DeviceCommunicationState::INIT_STATE;
					info.state = RectifierState::FAILED_TO_GET_STATE;
					return;
				}
			}
			else {
				info.communcationState = DeviceCommunicationState::INIT_STATE;
				info.state = RectifierState::FAILED_TO_GET_STATE;
				return;
			}
		}
		else {
			// OK
		}

	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
		return;
	}

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

	if (!rdSymbols.empty()) {
		DeviceCommand::parseResponseCode(rdSymbols, modbus_addr, modbus_func, reply_code);
		if (reply_code != 5) {
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return;
		}
		if (modbus_addr != info.address) {
			info.state = RectifierState::ADDRESS_DOESNT_MATCH;
			return;
		}
		// L"Send command 0x01";
		frameBytes = DeviceCommand::createCmdFrame(
			info.address, 0x43, DeviceCommand::GIVE_PREPARED_DATA_01);
		frameSymbols = DeviceCommand::convertToASCIIFrame(frameBytes);
		info.recivedData.buffer.clear();
		Device::sendCommand(frameSymbols, dwBytesWritten, log);
		Sleep(100);
		if (dwBytesWritten > 0) {
			info.communcationState = DeviceCommunicationState::FRAME_SENDED;
		}
		Sleep(1);
		std::uint8_t modbus_addr;
		std::uint8_t modbus_func;
		std::uint8_t reply_code;

		// wait for replay (should be read in separate  thread)
		int count = 0;
		while (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
			Sleep(100);
			if (count++ > 100) {
				break;
			}
		}

		if (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
			info.communcationState = DeviceCommunicationState::INIT_STATE;
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return;
		}

		if (info.communcationState == DeviceCommunicationState::REPLY_READ_TIMEOUT) {
			info.communcationState = DeviceCommunicationState::INIT_STATE;
			info.state = RectifierState::FAILED_TO_GET_STATE;
			return;
		}

		if (info.communcationState == DeviceCommunicationState::READ_SUCCEEDED) {
			// check the whole frame received
			rdSymbols.clear();
			rdSymbols.insert(rdSymbols.begin(), info.recivedData.buffer.begin(), info.recivedData.buffer.end());
			if (!isValidFrame(rdSymbols)) {
				readFromPort(rdSymbols, frameSymbols, dwBytesWritten, log);
				ss.clear();
				ss << dwBytesWritten << L" Bytes in string. " << std::endl << dwBytesWritten << L" Bytes sended. " << std::endl;
				str1 = ss.str();
				log += str1.c_str();
			}
			for (auto symbol : rdSymbols) {
				ss << std::hex << symbol;
			}
			ss << std::endl;
			str1 = ss.str();
			log += str1.c_str();
			DeviceCommand::REPLY_DATA data;
			DeviceCommand::parseResponseFrame(rdSymbols, modbus_addr, modbus_func, data);
			if (data.data.size() > 0 && info.address == modbus_addr) {
				if (plusTime) {
					DeviceCommand::StateF05 state = DeviceCommand::parseDataForF05(data.data);
					info.stateF05 = state;
				}
				else {
					DeviceCommand::StateF07 state = DeviceCommand::parseDataForF07(data.data);
					info.stateF05.setHiA(state.aHi);
					info.stateF05.setLowA(state.aLow);
					info.stateF05.setV(state.V);
				}
				info.state = RectifierState::OK;
			}
			else {
				if (info.address == modbus_addr) {
					info.state = RectifierState::DATA_ADDRESS_DOESNT_MATCH;
				}
				else {
					info.state = RectifierState::FAILED_TO_GET_STATE;
				}
			}
		}
	}
	else {
		info.state = RectifierState::FAILED_TO_GET_STATE;
	}
}

void Device::clearReciveBuffer()
{
	uint8_t buffer[1024];
	DWORD size;
	do {
		ReadFile(this->hSerial, buffer, 1024, &size, overlappedRDPtr);
	} while (size > 0);

}



UINT ThreadProc(LPVOID par) {
	SThread_param * param;
	param = (SThread_param *)par;
	param->statePtr[0] = 1;//выставили флаг, что поток работы с ком портом запущен
	
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
			RectifierInfo & info = rectInfo.second;
			// for now only one port for all rectifiers
			device.registerRectifier(info);
			//device.clearReciveBuffer();
			device.getRectifierState(info, true);
			info.recivedData.status = ++cnt;
			PostMessage((HWND)param->wnd, WM_COMMAND, 7, 7);
		}
		Sleep(1000);
	}
	param->statePtr[0] = 3;
	AfxEndThread(0);
	return 0;
}

UINT ReadingComPortThread(LPVOID par) {
	Reading_thread_param * param = (Reading_thread_param *)par;
	param->state[0] = 1; // thread reading port started
	int cnt = 0;
	std::wstringstream ss;
	ss << ++cnt;
	OVERLAPPED overlappedRD;
	overlappedRD.hEvent = CreateEvent(NULL, true, false, NULL);
	overlappedRD.Internal = 0;
	overlappedRD.InternalHigh = 0;
	overlappedRD.Offset = 0;
	overlappedRD.OffsetHigh = 0;
	DWORD mask;


	while (1) {
		if (param->state[0] == 2) {
			//команда на выход из потока работы с ком портом
			break;
		}
		// for now only one port for all rectifiers
		for (auto & rectInfo : param->m_rectifierConfigs[0]) {
			RectifierInfo & info = rectInfo.second;
			if (info.communcationState == DeviceCommunicationState::FRAME_SENDED) {
				// command sended try to read replay
				std::vector<std::uint8_t> symbols;
				DWORD trans;
				DWORD rdBytes;
				PortState portState;
				unsigned char bufrd[1024];
				BOOL res1 = ReadFile(info.hSerial, bufrd, 1024, &rdBytes, NULL);
				if (res1) {
					if (rdBytes > 0) {   // если что-то принято, выводим
						for (DWORD i = 0; i < rdBytes; ++i) {
							symbols.push_back(bufrd[i]);
						}
						portState = PortState::READ_SUCCEEDED;
					}
				}
				else {
					DWORD lastError = GetLastError();
//					if (portState == PortState::READ_TIMEOUT) {
					info.communcationState = DeviceCommunicationState::REPLY_READ_TIMEOUT;
				}

				if (portState == PortState::READ_SUCCEEDED) {
						// put read bytes in buffer
						info.recivedData.status += 1;
						info.recivedData.buffer.insert(
							info.recivedData.buffer.end(),
							symbols.begin(),
							symbols.end());
						info.communcationState = DeviceCommunicationState::READ_SUCCEEDED;
				}
				else if (portState == PortState::READ_TIMEOUT) {
					info.communcationState = DeviceCommunicationState::REPLY_READ_TIMEOUT;
				}



				//BOOL waitRes = WaitCommEvent(info.hSerial, &mask, &overlappedRD);
				//if (waitRes) {

				//}else {
				//	DWORD lastError = GetLastError();
				//	if (lastError == ERROR_IO_PENDING) {
				//		//проверяем маску эвентов на эвент принятия байт
				//		//unsigned char bufrd[1024];
				//		//DWORD rdBytes;
				//		//BOOL rdRes = ReadFile(info.hSerial, bufrd, 1024, NULL, &overlappedRD);
				//		//if (rdRes != 0) {
				//		//	DWORD lastError = GetLastError();
				//		//}
				//		//BOOL getRes = GetOverlappedResult(info.hSerial, &overlappedRD, &trans, false);
				//		if (WaitForSingleObject(overlappedRD.hEvent, INFINITE) == WAIT_OBJECT_0) {
				//			BOOL getRes = GetOverlappedResult(info.hSerial, &overlappedRD, &trans, true);
				//		}
				//	}
				//}
				//DWORD errors;
				//COMSTAT stat;
				////выполняем очищение ошибок с получением статуса порта
				//ClearCommError(info.hSerial, &errors, &stat);
				////проверяем маску эвентов на эвент принятия байт
				//

				//
				//if ((mask & EV_RXCHAR) == EV_RXCHAR) {
				//	//пытаемся прочитать максимальное количество байт
				//	BOOL res1 = ReadFile(info.hSerial, bufrd, 1024, &rdBytes, &overlappedRD);
				//	BOOL getRes = GetOverlappedResult(info.hSerial, &overlappedRD, &trans, false);
				//	if (rdBytes > 0) {   // если что-то принято, выводим
				//		for (DWORD i = 0; i < rdBytes; ++i) {
				//			symbols.push_back(bufrd[i]);
				//		}
				//		portState = PortState::READ_SUCCEEDED;
				//	}
				//}
				////portState = Comport::readPort(info.hSerial, &overlappedRD, &mask, symbols);
				//if (portState == PortState::READ_SUCCEEDED) {
				//	// put read bytes in buffer
				//	info.recivedData.status += 1;
				//	//a.insert(a.end(), b.begin(), b.end());
				//	info.recivedData.buffer.insert(
				//		info.recivedData.buffer.end(),
				//		symbols.begin(),
				//		symbols.end());
				//	info.communcationState = DeviceCommunicationState::READ_SUCCEEDED;
				//} else if (portState == PortState::READ_TIMEOUT){
				//	info.communcationState = DeviceCommunicationState::REPLY_READ_TIMEOUT;
				//}
				info.recivedData.status = ++cnt;
			}
			//PostMessage((HWND)param->wnd, WM_COMMAND, 7, 7);
		}
		Sleep(10);
	}
	param->state[0] = 3;
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
		bool waitRes = GetOverlappedResult(hSerial, overlappedRD, &numberOfBytesTransfered, true);
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
