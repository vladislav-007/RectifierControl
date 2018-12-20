#ifndef _SCOM_PORT
#define _SCOM_PORT
#include "stdafx.h"
#include <map>
#include <vector>
#include <afxmt.h>
#include "rectifiercommand.h"

const BYTE READ_MACHINE_STATE=0;
const BYTE WRITE_MACHINE_STATE=0x80;
const BYTE WRITE_DATA_TO_MACHINE=0xff;


//ошибки передачи и приема пакета
const WORD WRITE_ERROR = 1;
const WORD WRITE_RESULT_ERROR = 2;
const WORD WRITE_TIMEOUT_ERROR = 4;
const WORD READ_ERROR = 0x8;
const WORD READ_RESULT_ERROR = 0x10;
const WORD READ_TIMEOUT_ERROR = 0x20;
const WORD ADDRESS_ERROR = 0x40;
const WORD NO_BURST_STOP = 0x80;
const WORD BAD_CHECK_SUMM= 0x0100;






const BYTE START_BYTE = 0xAA;
const BYTE STOP_BYTE = 0xAB;
const BYTE SHIFT_BYTE = 0xAC;
//состояние загрузки
const WORD TASK_NONE=	0;
const WORD TASK_SET	=	1;
const WORD TASK_STARTED=2;
const WORD TASK_FINISHED=3;
const WORD TASK_CLOSED=4;
const WORD TASK_RESET=6;



//состояние контроллера станка
const BYTE MACH_FS_req	=	1;
const BYTE MACH_FS_busy	=	2;
const BYTE MACH_FS_ovfl	=	4;
const BYTE MACH_PL_eof	=	0x40;
const BYTE MACH_PL_req	=	0x80;

enum class Parity : std::int8_t {
	NO_PARITY = 0,
	ODD_PARITY = 1,
	EVEN_PARITY = 2,
	MARK_PARITY = 3,
	SPACE_PARITY = 4
};





enum class Stopbits : std::int8_t {
	ONE_STOPBIT = 0,
	ONE5_STOPBITS = 1,
	TWO_STOPBITS = 2
};

enum class RectifierState : std::int8_t {
	OK = 0,
	FAILED_TO_OPEN_COMPORT = 1,
	FAILED_TO_GET_STATE = 2,
	INVALID_USER_BUFFER = 3,
	UNKNOWN_ERROR = 4,
	NOT_INITIALIZED = 5,
	ADDRESS_DOESNT_MATCH = 6,
	DEVICE_ISNT_READY = 7
};

//template<class T>
//std::ostream& operator<<(std::ostream& os, T enumValue)
//{
//	return os << toString(enumValue);
//}



//std::ostream& operator<<(std::ostream& os, RectifierState enumValue)
//{
//	return os << toString(enumValue);
//}

struct CmdsToSend {
	int status;
	std::int8_t cmd1[256];
	std::int8_t cmd2[256];
};

struct RecivedData {
	int status;
	std::int8_t buffer[256];
};

struct RectifierInfo {
	RectifierState state = RectifierState::NOT_INITIALIZED;
	//DeviceCommand::StateF07 stateF07;
	DeviceCommand::StateF05 stateF05;
	int id;
	CString name;
	int address;
	CString comport;
	HANDLE hSerial;
	int modeID;
	CString modeName;
	int modeBoundRate;
	int modeByteSize;
	Parity modeParity;
	Stopbits modeStopbits;
	CmdsToSend cmdToSend;
	RecivedData recivedData;
	CDocument * doc;
};


class Device {
	static std::map<CString, HANDLE> openedPorts;
	static std::map<HANDLE, int> openedPortsCount;
public:
	//Device(RectifierInfo & info);
	Device(RectifierInfo & info, OVERLAPPED * const stateDialogOverlappedRD, DWORD * pMask, OVERLAPPED * const stateDialogOverlappedWR);
	DWORD WaitForReadSingleObject(DWORD timeout);
	BOOL resetReadEvent();
	~Device();
	void getFrameFromBuffer(std::vector<std::uint8_t>& rdSymbols);
	static const CString & toString(RectifierState state);
private:
	Device() = delete;
	Device & operator=(Device&) = delete;

public:
	RectifierState readFromPort(std::vector<std::uint8_t>& rdSymbols);
	void getReadFrameFromPort(DWORD signal, std::vector<std::uint8_t> & rdSymbols);
	void sendCommand(std::vector<uint8_t> & frameSymbols, DWORD & dwBytesWritten, CString &log);
	void sendReplyData(std::vector<uint8_t> frameSymbols, DWORD & dwBytesWritten, CString & log);
	static bool isValidFrame(std::vector<std::uint8_t> & symbols);
	static bool trimLeftSymbolsSequenceAsFrame(std::vector<std::uint8_t>& framePretenders);
	static std::vector<std::uint8_t> Device::getFrameFromTail(std::vector<std::uint8_t> & symbolsTail);
	//void getRectifierState(RectifierInfo & info);

	void getRectifierState(RectifierInfo & info, bool plusTime);

	void clearReciveBuffer();
private:
	HANDLE hSerial;
	OVERLAPPED * overlappedRDPtr;
	OVERLAPPED * overlappedWRPtr;
	DWORD * mask;
	std::vector<std::uint8_t> symbolsTail;
	
};

struct SThread_param {
	HWND wnd;
	WORD * statePtr;
	//CCriticalSection * portBlock;
	//DWORD * dwpByteTimeOut;
	std::map<int, RectifierInfo> * m_rectifierConfigs;
	//OVERLAPPED * Sync;
	OVERLAPPED * mainOverlappedRD;
	DWORD * pMask;
	OVERLAPPED * mainOverlappedWR;
};



struct MachineTaskState {
	WORD wTaskState;
	CString * taskFileName;
	CStdioFile * taskCFile;
	BYTE bMachineState;
	WORD wNetError;
	bool shouldInitByFF;
	};

class ComPort{
public:
	ComPort();
	ComPort( HANDLE port );
	~ComPort();

	void setPortHandle( HANDLE port ){
		portBlock.Lock();
		portHandle = port;
		portBlock.Unlock();
	}

	HANDLE getPortHandle(){
		return portHandle;
		}

	void setWnd( HWND aWnd){
		wnd = aWnd;
		}

	void clearError(){
		error = 0;
		}
	WORD getError();

private:
	BOOL lock();
private:
	CCriticalSection portBlock;
	HANDLE portHandle;
	HWND wnd;
	DWORD dwByteTimeOut; 
	WORD error;
};



UINT ThreadProc(LPVOID par );

#endif