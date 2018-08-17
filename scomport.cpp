#include "stdafx.h"
#include "scomport.h"
#include "RectifierCommand.h"
#include <vector>

//#include "cncload.h"

#include <Windows.h>
#include <sstream>

OVERLAPPED overlappedWR;
OVERLAPPED overlappedRD;

char * taskStateStr[] = { "Нет задания","Ожидание запроса на загрузку программы",
	"Загрузка программы",
	"Загрузка программы завершена", "","" };

char * machStateStr[] = { " Режим ожидания","Старт фотоввода","", NULL };

//WORD threadState;
//MachineTaskState machinesTasks[16];

//CSComPort::CSComPort() {
//	portHandle = NULL;
//	error = 0;
//	wnd = NULL;
//}
//
//WORD CSComPort::makeDataBurst(const BYTE address, const BYTE cmd, const BYTE * data, BYTE length, BYTE burst[136]) {	//
//	BYTE checkSum;
//	burst[0] = 0xAA;//START
//	burst[1] = address;
//	burst[2] = cmd;
//	int i, j;
//
//	if (data == NULL) {
//		length = 0;
//	}
//
//	for (i = 0, j = 3; i < length; i++) {
//		if ((data[i] != 0xAC) && (data[i] != 0xAB) && (data[i] != 0xAA)) {//AC = SHIFT AB=STOP AA=START 
//			burst[j] = data[i];
//			j++;
//		}
//		else {
//			burst[j] = 0xAC;
//			j++;
//			burst[j] = data[i] - 0xAA;
//			j++;
//		}
//	}
//	//теперь добавим контрольную сумму
//	checkSum = 0;
//
//	for (i = 1; i < j; i++) {
//		checkSum ^= burst[i];
//	}
//
//	if ((checkSum != 0xAC) && (checkSum != 0xAB) && (checkSum != 0xAA)) {//AC = SHIFT AB=STOP AA=START 
//		burst[j] = checkSum;
//		j++;
//	}
//	else {
//		burst[j] = 0xAC;
//		j++;
//		burst[j] = checkSum - 0xAA;
//		j++;
//	}
//	burst[j] = 0xAB;
//	j++;
//	return j;
//}
//
//WORD CSComPort::parseMachineBurst(const BYTE * burst, BYTE length, BYTE & address, BYTE * data) {
//	//
//	BYTE checkSum, stopByte;
//	WORD i, j;
//
//	if (length == 0) {
//		return 0;
//	}
//
//	if (burst == NULL) {
//		return 0;
//	}
//	else {
//		//ответный пакет не содержит байт START, начинается с адреса
//		address = burst[0];
//	}
//
//	stopByte = 0;
//	i = 1;
//	j = 0;
//	for (; i < length; i++) {
//		if (burst[i] == 0xAB) {
//			stopByte = burst[i];
//			//конец пакета
//			break;
//		}
//
//		if (burst[i] != 0xAC) {//AC = SHIFT AB=STOP AA=START 
//			data[j] = burst[i];
//		}
//		else {
//			i++;
//			data[j] = burst[i] + 0xAA;
//		}
//		j++;
//	}
//
//	if (stopByte != 0xAB) {
//		error |= NO_BURST_STOP;
//	}
//	//теперь проверим контрольную сумму
//	checkSum = address;
//	for (i = 0; i < j; i++) {
//		checkSum ^= data[i];
//	}
//
//	if (checkSum) {
//		error |= BAD_CHECK_SUMM;
//	}
//	//уменьшим количество меньше на 1 из-за контрольной суммы
//	j -= 1;
//	return j;
//}
//
//WORD CSComPort::getError() {
//	WORD aError;
//	aError = error;
//	return error;
//}
//
////это вариант синхронной работы с ком портом
//WORD CSComPort::readPort(BYTE * buff, DWORD size, BYTE endByte) {
//	BYTE byte;
//	DWORD dwRead, localError;
//	WORD index;
//	DWORD lastError;
//	index = 0;
//	byte = 0;
//	localError = 0;
//	while (index < size) {
//		//читаем пока не примем стоповый байт или size байт
//		//читаем по одному байту
//		if (!ReadFile(portHandle, &byte, 1, &dwRead, NULL)) {
//			//пока чтение не закончено
//			lastError = GetLastError();
//			DWORD code;
//			code = GetLastError();
//			CString cs;
//			cs.Format(L"Code=%d", code);
//			//::MessageBox((HWND) wnd, cs, "Thread", MB_OK );
//			localError |= READ_ERROR;
//		}
//		else {
//			if (dwRead) {
//				buff[index] = byte;
//				index++;
//			}
//			else {
//				localError |= READ_TIMEOUT_ERROR;
//			}
//		}
//
//		if (localError) {
//			break;
//		}
//		if (byte == endByte)
//			break;
//	}
//	error |= localError;
//	return index;
//}
//
//
//BYTE CSComPort::getMachineState(BYTE address) {
//	BYTE outBuff[160], inpBuff[142], data[64], state;
//	WORD length, localError;
//	CString message;
//	DWORD dwWrite;
//	DWORD result;
//	//OVERLAPPED Sync = {0};
//	//OVERLAPPED SyncRead = {0};
//	BYTE recivedAddress;
//	//char str[128];
//	state = 0;
//	localError = 0;
//	length = makeDataBurst(address, READ_MACHINE_STATE, 0, 0, outBuff);
//	SetLastError(0);
//	if (!WriteFile(portHandle, outBuff, (DWORD)length, &dwWrite, NULL)) {
//		result = GetLastError();
//		CString cs;
//		cs.Format("Code=%d", result);
//		::MessageBox((HWND)wnd, cs, "Thread", MB_OK);
//		// Ошибка выполения операции 
//		error |= WRITE_RESULT_ERROR;
//	}
//	else {
//		if (length == dwWrite) {
//			length = readPort(inpBuff, 136);
//			length = parseMachineBurst(inpBuff, (BYTE)length, recivedAddress, data);
//			if (address != recivedAddress) {
//				localError |= ADDRESS_ERROR;
//			}
//			if ((localError == 0) && (length)) {
//				//::MessageBox((HWND) wnd, "OK", "Thread", MB_OK );
//				state = data[0];
//			}
//		}
//		else {
//			error |= WRITE_TIMEOUT_ERROR;
//		}
//	}
//	error |= localError;
//	return state;
//}
//
//BYTE CSComPort::setMachineState(BYTE address, BYTE aState) {
//	BYTE outBuff[160], inpBuff[142], data[64], state;
//	WORD length, localError;
//	CString message;
//	DWORD dwWrite;
//	DWORD result;
//	//OVERLAPPED Sync = {0};
//	//OVERLAPPED SyncRead = {0};
//	BYTE recivedAddress;
//	//char str[128];
//	BYTE sendState[1];
//
//	state = 0;
//	localError = 0;
//	sendState[0] = aState;
//
//
//	length = makeDataBurst(address, WRITE_MACHINE_STATE, sendState, 1, outBuff);
//	SetLastError(0);
//	if (!WriteFile(portHandle, outBuff, (DWORD)length, &dwWrite, NULL)) {
//		result = GetLastError();
//		CString cs;
//		cs.Format("Code=%d", result);
//		//::MessageBox((HWND) wnd, cs, "Thread", MB_OK );
//		// Ошибка выполения операции 
//		error |= WRITE_RESULT_ERROR;
//	}
//	else {
//		if (length == dwWrite) {
//			length = readPort(inpBuff, 136);
//			length = parseMachineBurst(inpBuff, (BYTE)length, recivedAddress, data);
//			if (address != recivedAddress) {
//				localError |= ADDRESS_ERROR;
//			}
//			if ((localError == 0) && (length)) {
//				//::MessageBox((HWND) wnd, "OK", "Thread", MB_OK );
//				state = data[0];
//			}
//		}
//		else {
//			error |= WRITE_TIMEOUT_ERROR;
//		}
//	}
//	error |= localError;
//	return state;
//}
//
//
//BYTE CSComPort::writeDataToMachine(const BYTE address, const BYTE * aData, const UINT aLength) {
//	BYTE outBuff[160], inpBuff[136], data[64], state;
//	WORD length, localError;
//	CString message;
//	DWORD dwWrite;
//	//OVERLAPPED Sync = {0};
//	//OVERLAPPED SyncRead = {0};
//	BYTE recivedAddress;
//	//char str[128];
//
//	state = 0;
//	localError = 0;
//
//	length = makeDataBurst(address, WRITE_DATA_TO_MACHINE, aData, aLength, outBuff);
//	//sprintf( str, "сформировали пакет данных(%ld):%x %x %x %x %x", length, outBuff[0], outBuff[1], outBuff[2], outBuff[3], outBuff[4] );	
//	//::MessageBox((HWND) wnd, str, "Thread", MB_OK );
//
//	if (!WriteFile(portHandle, outBuff, (DWORD)length, &dwWrite, NULL)) {
//	}
//	else {
//		if (length == dwWrite) {
//			//::MessageBox((HWND) wnd, "Послан буфер", "Thread", MB_OK );
//			length = readPort(inpBuff, 136);
//
//			length = parseMachineBurst(inpBuff, (BYTE)length, recivedAddress, data);
//			if (address != recivedAddress) {
//				localError |= ADDRESS_ERROR;
//			}
//			if ((localError == 0) && (length)) {
//				//::MessageBox((HWND) wnd, "OK", "Thread", MB_OK );
//				state = data[0];
//			}
//		}
//		else {
//			localError = WRITE_TIMEOUT_ERROR;
//		}
//	}
//
//	error |= localError;
//	return state;
//}



//UINT ThreadProc(LPVOID par ){
//	SThread_param * param;
//	CSComPort comPort;
//	param = (SThread_param *)par;
//	//dwByteTimeOut = (DWORD) (*param->dwpByteTimeOut);
//	threadState = 1;//выставили флаг, что поток работы с ком портом запущен
//	static int s;
//	static int st;
//	static HANDLE tstPortHandle;
//	st = 0;
//	s =0;
//	static int flag =0;
//
//	for( int i=0;i<16;i++){
//		machinesTasks[i].wTaskState = 0;
//		}	
//	//::MessageBox((HWND) param->wnd,"Thread started", "Thread", MB_OK);
//	comPort.setPortHandle( param->portPtr[0] );
//	tstPortHandle = param->portPtr[0];
//	//getMachineState( param->portPtr[0], 1, param->wnd, &error, 200 );
//
//
//
//
//	while(1){
//		if( threadState == 2 ){
//			//команда на выход из потока работы с ком портом
//			break;
//			}
//		//теперь заблокируем доступ к компорту, чтобы его параметры невозможно было изменить
//		//во время работы с ним
//		
//		/*
//		if( tstPortHandle != param->portPtr[0] ){
//			::MessageBox((HWND) param->wnd,"Port was changed!!!", "Thread", MB_OK);
//			tstPortHandle = param->portPtr[0];
//			if( param->portPtr[0] == NULL )
//				::MessageBox((HWND) param->wnd,"Port is NULL !!!", "Thread", MB_OK);
//			}
//
//		if( param->statePtr[0] ){
//			comPort.setPortHandle( param->portPtr[0] );
//			if( flag ){ 
//				::MessageBox((HWND) param->wnd,"State is not 0 ", "Thread", MB_OK);	
//				flag =0;
//				}
//			}
//		else {
//			flag 
//			}
//		
//		param->portBlock->Unlock();
//		}
//		*/
//		if( param->statePtr[0] ){
//			if( flag == 0){
//				//::MessageBox((HWND) param->wnd, "Состояние  != 0" , "Thread", MB_OK);
//				flag = 1;
//				}
//			
//			param->portBlock->Lock();
//			//сначала опросим те станки, для котрых задано задание
//
//			DWORD dwIsWorking;//флаг наличия задание загрузки и готовности какого-то из станков.
//			//сначала опросим станки, для которых задано задание
//			BYTE buf[264];
//			dwIsWorking = 0;
//			int i;
//
//			for( i=1;i<16;i++){
//				if( machinesTasks[i].wTaskState != TASK_NONE ){
//					//CString str;
//					//str.Format("wTaskState = %02x", machinesTasks[i].wTaskState );
//					//AfxMessageBox( str );
//					//flag = 1;
//					//задача работает или поставлена
//					//опросим состояние
//					comPort.clearError();
//					if( (machinesTasks[i].wTaskState == TASK_SET) || (machinesTasks[i].wTaskState == TASK_RESET) ){
//						machinesTasks[i].bMachineState = comPort.setMachineState( i );
//						if( machinesTasks[i].wTaskState == TASK_RESET ){
//							machinesTasks[i].wTaskState = TASK_NONE;
//							}
//						}
//					else {
//						machinesTasks[i].bMachineState = comPort.getMachineState( i );
//						}
//
//					machinesTasks[i].wNetError = comPort.getError();
//					comPort.clearError();
//					
//					if( machinesTasks[i].wTaskState==TASK_SET ){
//						// 27.07.09 add init befor load
//						if( machinesTasks[i].shouldInitByFF == true )
//						{
//							BYTE initByte = 0xff;
//							comPort.writeDataToMachine( i, &initByte, 1 );
//							machinesTasks[i].shouldInitByFF = false;
//						}
//
//						//задача постановлена
//						if( machinesTasks[i].bMachineState&MACH_FS_req ){
//							//был запрос на прием из компьютера
//							machinesTasks[i].wTaskState = TASK_STARTED;
//							machinesTasks[i].taskCFile->Seek(0,0);
//							}
//						}
//					if( machinesTasks[i].wTaskState == TASK_STARTED ){
//						//загрузка началась
//						dwIsWorking++;
//						//BYTE burst[136];
//						UINT nBytesRead;
//						if( (machinesTasks[i].bMachineState&MACH_FS_busy)==0 ){
//							//enlage buffer from 16 to 64 27.07.09
//							nBytesRead = machinesTasks[i].taskCFile->Read( buf, 64 );
//							/*	//сделаем байты четными 
//
//							unsigned char ch;
//							for( UINT j=0; j < nBytesRead;j++){
//								ch = buf[j];
//								for(int i=4;i>=1;i=(i>>1)){
//									ch^=ch<<i;
//									}
//								buf[j]= buf[j]^(ch&0x80);
//								}
//								*/
//
//							comPort.writeDataToMachine( i, buf, nBytesRead);
//							if( (nBytesRead==0) ){
//								//файл считан полностью
//								machinesTasks[i].wTaskState = TASK_FINISHED;
//								}
//							}
//						}
//					}
//				}
//				
//				
//			if( dwIsWorking == 0 ){
//				for( i=1;i<16;i++){
//					comPort.clearError();
//
//
//					if( (machinesTasks[i].wTaskState == TASK_SET) || (machinesTasks[i].wTaskState == TASK_RESET) ){
//						machinesTasks[i].bMachineState = comPort.setMachineState( i );
//						if( machinesTasks[i].wTaskState == TASK_RESET ){
//							machinesTasks[i].wTaskState = TASK_NONE;
//							}
//						}
//					else {
//						machinesTasks[i].bMachineState = comPort.getMachineState( i );
//						}
//
//					machinesTasks[i].wNetError = comPort.getError();
//					comPort.clearError();
//									
//					if( machinesTasks[i].wTaskState==TASK_SET ){
//						//задача постановлена
//						if( machinesTasks[i].bMachineState&MACH_FS_req ){
//							//запрос на прием из компьютера
//							machinesTasks[i].wTaskState = TASK_STARTED;
//							machinesTasks[i].taskCFile->Seek(0,0);
//							}
//						} 
//	
//					if( machinesTasks[i].wTaskState == TASK_STARTED ){
//						BYTE buf[256];
//						//BYTE burst[136];
//						UINT nBytesRead;
//						if( (machinesTasks[i].bMachineState&MACH_FS_busy)==0 ){
//							//enlage buffer from 16 to 64 27.07.09
//							nBytesRead = machinesTasks[i].taskCFile->Read( buf, 64 );
//							/*
//							//сделаем байты четными 
//							unsigned char ch;
//							for( UINT j=0; j < nBytesRead;j++){
//								ch = buf[j];
//								for(int i=4;i>=1;i=(i>>1)){
//									ch^=ch<<i;
//									}
//								buf[j]= buf[j]^(ch&0x80);
//								}
//							*/
//							if( (nBytesRead==0) ){
//								//файл считан полностью
//								machinesTasks[i].wTaskState = TASK_FINISHED;
//								}
//							else {
//								comPort.writeDataToMachine( i, buf, nBytesRead);
//								}
//							}
//			
//						}
//					
//					}
//				}
//			param->portBlock->Unlock();
//			}	
//		else {
//			if( flag ){
//				//::MessageBox((HWND) param->wnd, "Состояние  = 0" , "Thread", MB_OK);
//				flag = 0;
//				}
//			Sleep(10);
//			}
//		//теперь заблокируем доступ к компорту, чтобы его параметры невозможно было изменить
//		//во время работы с ним
//		//разблокируем
//		
//		/*
//		if( param->statePtr[0] == 0 ){
//			Sleep(100);
//			}
//		else {
//			
//			}
//		*/
//		}
//	//::MessageBox((HWND) param->wnd, "Поток остановлен", "Thread", MB_OK);
//	
//
//	//::MessageBox(NULL,"Thread stopted", "Thread", MB_OK);
//	threadState = 3;
//	AfxEndThread( 0 );
//	return 0;
//	}

void getRectifierState(RectifierInfo & info) {

	// L"Send command 0x07";
	std::vector<uint8_t> frameBytes = DeviceCommand::createCmdFrame(
		info.address, 0x43, GET_CONCISE_DEVICE_STATE_07);

	frameBytes = DeviceCommand::convertToASCIIFrame(frameBytes);

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
		info.state = RectifierState::FAILED_TO_OPEN_COMPORT;
		AfxMessageBox(message, MB_YESNO | MB_ICONSTOP);
		return;
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
	//log += str1.c_str();
	//m_CEditTestLog.SetWindowText(log);
	Sleep(150);

	ss.clear();
	DWORD iSize;
	//char sReceivedChar;
	ss << L"Started reading replay..." << std::endl;
	str1 = ss.str();
	//log += str1.c_str();
	//m_CEditTestLog.SetWindowText(log);
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
							for (DWORD i = 0; i < iSize; ++i) {
								ss << std::hex << bufrd[i];
								info.recivedData.buffer[i] = bufrd[i];
							}
							info.recivedData.buffer[iSize] = 0;
						}
					}
				}
		}
		else {
			//log += L"НЕТ ОТВЕТА!!!";
			//m_CEditTestLog.SetWindowText(log);
			AfxMessageBox(L"НЕТ ОТВЕТА", MB_YESNO | MB_ICONSTOP);
			break;
		}
		break;
	}
	ss << std::endl;
	str1 = ss.str();
	
	//log += str1.c_str();
	//m_CEditTestLog.SetWindowText(log);
	CloseHandle(hSerial);
}

UINT ThreadProc(LPVOID par) {
	SThread_param * param;
	//CSComPort comPort;
	param = (SThread_param *)par;
	
	////dwByteTimeOut = (DWORD) (*param->dwpByteTimeOut);
	param->statePtr[0] = 1;//выставили флаг, что поток работы с ком портом запущен
	std::map<int, RectifierInfo> * rectInfos = param->m_rectifierConfigs;

	//static int s;
	//static int st;
	//static HANDLE tstPortHandle;
	//st = 0;
	//s = 0;
	//static int flag = 0;

	//for (int i = 0; i<16; i++) {
	//	machinesTasks[i].wTaskState = 0;
	//}
	::MessageBox((HWND)param->wnd, L"Thread started", L"Thread", MB_OK);
	//comPort.setPortHandle(param->portPtr[0]);
	//		tstPortHandle = param->portPtr[0];
			//getMachineState( param->portPtr[0], 1, param->wnd, &error, 200 );

	int cnt = 0;
	std::wstringstream ss;
	ss << ++cnt;
	

	while (1) {
		if (param->statePtr[0] == 2) {
			//команда на выход из потока работы с ком портом
			break;
		}
		Sleep(1000);
		for (auto & rectInfo : rectInfos[0]) {
			RectifierInfo & info = rectInfo.second;
			getRectifierState(info);
			info.recivedData.status = ++cnt;
			PostMessage((HWND)param->wnd, WM_COMMAND, 7,7);
		}
		//std::wstringstream ss;
		//ss << L"Thread cnt: " << ++cnt;
		//::MessageBox((HWND)param->wnd, L"Thread started", ss.str().c_str() , MB_OK);
		//теперь заблокируем доступ к компорту, чтобы его параметры невозможно было изменить
		//во время работы с ним

		/*
		if( tstPortHandle != param->portPtr[0] ){
		::MessageBox((HWND) param->wnd,"Port was changed!!!", "Thread", MB_OK);
		tstPortHandle = param->portPtr[0];
		if( param->portPtr[0] == NULL )
		::MessageBox((HWND) param->wnd,"Port is NULL !!!", "Thread", MB_OK);
		}

		if( param->statePtr[0] ){
		comPort.setPortHandle( param->portPtr[0] );
		if( flag ){
		::MessageBox((HWND) param->wnd,"State is not 0 ", "Thread", MB_OK);
		flag =0;
		}
		}
		else {
		flag
		}

		param->portBlock->Unlock();
		}
		*/
		//if (param->statePtr[0]) {
		//	if (flag == 0) {
		//		//::MessageBox((HWND) param->wnd, "Состояние  != 0" , "Thread", MB_OK);
		//		flag = 1;
		//	}

		//	param->portBlock->Lock();
		//	//сначала опросим те станки, для котрых задано задание

		//	DWORD dwIsWorking;//флаг наличия задание загрузки и готовности какого-то из станков.
		//					  //сначала опросим станки, для которых задано задание
		//	BYTE buf[264];
		//	dwIsWorking = 0;
		//	int i;

		//	for (i = 1; i < 16; i++) {
		//		if (machinesTasks[i].wTaskState != TASK_NONE) {
		//			//CString str;
		//			//str.Format("wTaskState = %02x", machinesTasks[i].wTaskState );
		//			//AfxMessageBox( str );
		//			//flag = 1;
		//			//задача работает или поставлена
		//			//опросим состояние
		//			comPort.clearError();
		//			if ((machinesTasks[i].wTaskState == TASK_SET) || (machinesTasks[i].wTaskState == TASK_RESET)) {
		//				machinesTasks[i].bMachineState = comPort.setMachineState(i);
		//				if (machinesTasks[i].wTaskState == TASK_RESET) {
		//					machinesTasks[i].wTaskState = TASK_NONE;
		//				}
		//			}
		//			else {
		//				machinesTasks[i].bMachineState = comPort.getMachineState(i);
		//			}

		//			machinesTasks[i].wNetError = comPort.getError();
		//			comPort.clearError();

		//			if (machinesTasks[i].wTaskState == TASK_SET) {
		//				// 27.07.09 add init befor load
		//				if (machinesTasks[i].shouldInitByFF == true)
		//				{
		//					BYTE initByte = 0xff;
		//					comPort.writeDataToMachine(i, &initByte, 1);
		//					machinesTasks[i].shouldInitByFF = false;
		//				}

		//				//задача постановлена
		//				if (machinesTasks[i].bMachineState&MACH_FS_req) {
		//					//был запрос на прием из компьютера
		//					machinesTasks[i].wTaskState = TASK_STARTED;
		//					machinesTasks[i].taskCFile->Seek(0, 0);
		//				}
		//			}
		//			if (machinesTasks[i].wTaskState == TASK_STARTED) {
		//				//загрузка началась
		//				dwIsWorking++;
		//				//BYTE burst[136];
		//				UINT nBytesRead;
		//				if ((machinesTasks[i].bMachineState&MACH_FS_busy) == 0) {
		//					//enlage buffer from 16 to 64 27.07.09
		//					nBytesRead = machinesTasks[i].taskCFile->Read(buf, 64);
		//					/*	//сделаем байты четными

		//					unsigned char ch;
		//					for( UINT j=0; j < nBytesRead;j++){
		//					ch = buf[j];
		//					for(int i=4;i>=1;i=(i>>1)){
		//					ch^=ch<<i;
		//					}
		//					buf[j]= buf[j]^(ch&0x80);
		//					}
		//					*/

		//					comPort.writeDataToMachine(i, buf, nBytesRead);
		//					if ((nBytesRead == 0)) {
		//						//файл считан полностью
		//						machinesTasks[i].wTaskState = TASK_FINISHED;
		//					}
		//				}
		//			}
		//		}
		//	}


		//	if (dwIsWorking == 0) {
		//		for (i = 1; i < 16; i++) {
		//			comPort.clearError();


		//			if ((machinesTasks[i].wTaskState == TASK_SET) || (machinesTasks[i].wTaskState == TASK_RESET)) {
		//				machinesTasks[i].bMachineState = comPort.setMachineState(i);
		//				if (machinesTasks[i].wTaskState == TASK_RESET) {
		//					machinesTasks[i].wTaskState = TASK_NONE;
		//				}
		//			}
		//			else {
		//				machinesTasks[i].bMachineState = comPort.getMachineState(i);
		//			}

		//			machinesTasks[i].wNetError = comPort.getError();
		//			comPort.clearError();

		//			if (machinesTasks[i].wTaskState == TASK_SET) {
		//				//задача постановлена
		//				if (machinesTasks[i].bMachineState&MACH_FS_req) {
		//					//запрос на прием из компьютера
		//					machinesTasks[i].wTaskState = TASK_STARTED;
		//					machinesTasks[i].taskCFile->Seek(0, 0);
		//				}
		//			}

		//			if (machinesTasks[i].wTaskState == TASK_STARTED) {
		//				BYTE buf[256];
		//				//BYTE burst[136];
		//				UINT nBytesRead;
		//				if ((machinesTasks[i].bMachineState&MACH_FS_busy) == 0) {
		//					//enlage buffer from 16 to 64 27.07.09
		//					nBytesRead = machinesTasks[i].taskCFile->Read(buf, 64);
		//					/*
		//					//сделаем байты четными
		//					unsigned char ch;
		//					for( UINT j=0; j < nBytesRead;j++){
		//					ch = buf[j];
		//					for(int i=4;i>=1;i=(i>>1)){
		//					ch^=ch<<i;
		//					}
		//					buf[j]= buf[j]^(ch&0x80);
		//					}
		//					*/
		//					if ((nBytesRead == 0)) {
		//						//файл считан полностью
		//						machinesTasks[i].wTaskState = TASK_FINISHED;
		//					}
		//					else {
		//						comPort.writeDataToMachine(i, buf, nBytesRead);
		//					}
		//				}

		//			}

		//		}
		//	}
		//	param->portBlock->Unlock();
		//}
		//else {
		//	if (flag) {
		//		//::MessageBox((HWND) param->wnd, "Состояние  = 0" , "Thread", MB_OK);
		//		flag = 0;
		//	}
		//	Sleep(10);
		//}
		//теперь заблокируем доступ к компорту, чтобы его параметры невозможно было изменить
		//во время работы с ним
		//разблокируем

		/*
		if( param->statePtr[0] == 0 ){
		Sleep(100);
		}
		else {

		}
		*/
	}
	//::MessageBox((HWND) param->wnd, L"Поток остановлен", L"Thread", MB_OK);


	//::MessageBox(NULL, L"Thread stopted", L"Thread", MB_OK);
	param->statePtr[0] = 3;
	AfxEndThread(0);
	return 0;
}


