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
	DATA_ADDRESS_DOESNT_MATCH = 7,
	DEVICE_ISNT_READY = 8,
	FAILED_TO_SET_REMOTE_CONTROL = 9,
	FAILED_TO_SET_VOLTAGE = 10,
	FAILED_TO_STOP_EXECUTING_PROGRAMM = 11,
	FAILED_TO_START_EXECUTING_PROGRAMM = 12,
	FAILED_TO_TEST_POWER_MODUES = 13,
	SUCCESSFULLY_INITIALIZED = 14,
	FAILED_TO_READ_PROGRAM = 15,
	FAILED_TO_GET_DELAYED_REPLY = 16,
};

enum class PortState : std::int8_t {
	PORT_UNKNOWN = 0,
	PORT_READY = 1,
	FRAME_SENDED = 2,
	READ_TIMEOUT = 4,
	READ_EMPTY_BUFFER = 5,
	READ_SUCCEEDED = 7,
	READ_ERROR = 8,
};

enum class DeviceCommunicationState : std::int8_t {
	INIT_STATE = 0,
	FRAME_SENT = 2,
	REPLY_FRAME_RECEIVED = 3,
	REPLY_READ_TIMEOUT = 4,
	READ_SUCCEEDED = 7
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

enum class RectifierCmd : std::int8_t {
	SET_VOLTAGE_AND_CURRENT = 0,
	START_EXISTIN_PROGRAM = 1,
	STOP_EXISTIN_PROGRAM = 2,
};

enum class CmdStatus: std::int8_t {
	EMPTY = 0,
    PREPARED = 1,
	SENT = 2,
	SUCCEEDED = 3,
	FAILED = 4
};



struct CmdToExecute {
	CmdStatus status;
	RectifierCmd rectifierCmd;
	float voltage;
	float current;
};

struct RecivedData {
	int status;
	std::vector<std::int8_t> buffer;
};

struct RectifierInfo {
	RectifierState state = RectifierState::NOT_INITIALIZED;
	int badStateSkipCount = 0;
	DeviceCommunicationState communicationState = DeviceCommunicationState::INIT_STATE;
	PortState portState = PortState::PORT_UNKNOWN;
	OVERLAPPED * overlappedRD;
	OVERLAPPED * overlappedWR;
	DWORD * mask;
	DeviceCommand::StateF05 stateF05;
	std::vector<uint8_t> memory;
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
	CmdToExecute cmdToSend;
	RecivedData recivedData;
	CDocument * doc;
	static CCriticalSection cs;
};


class Device {
	static std::map<CString, HANDLE> openedPorts;
	static std::map<HANDLE, int> openedPortsCount;
public:
	//Device(RectifierInfo & info);
	Device(OVERLAPPED * const stateDialogOverlappedRD, DWORD * pMask, OVERLAPPED * const stateDialogOverlappedWR);
	bool registerRectifier(RectifierInfo & info);
	DWORD WaitForReadSingleObject(DWORD timeout);
	BOOL resetReadEvent();
	~Device();
	void getFrameFromBuffer(std::vector<std::uint8_t>& rdSymbols);
	static const CString & toString(RectifierState state);
private:
	Device() = delete;
	Device & operator=(Device&) = delete;

public:
	//RectifierState readFromPort(std::vector<std::uint8_t>& rdSymbols);
	void getReadFrameFromPort(DWORD signal, std::vector<std::uint8_t> & rdSymbols);
	static void sendCommand(HANDLE hSerial, std::vector<uint8_t> & frameSymbols, DWORD & dwBytesWritten, CString &log);
	void sendReplyData(std::vector<uint8_t> frameSymbols, DWORD & dwBytesWritten, CString & log);
	RectifierState readFromPort(std::vector<std::uint8_t>& rdSymbols, std::vector<uint8_t>& sendSymbols, DWORD & dwBytesWritten, CString & log);
	RectifierState readFromPort(std::vector<std::uint8_t>& rdSymbols);
	static bool isValidFrame(std::vector<std::uint8_t> & symbols);
	static bool trimLeftSymbolsSequenceAsFrame(std::vector<std::uint8_t>& framePretenders);
	static std::vector<std::uint8_t> Device::getFrameFromTail(std::vector<std::uint8_t> & symbolsTail);
	//void getRectifierState(RectifierInfo & info);

	bool getRectifierState(RectifierInfo & info, bool plusTime);

	bool executeCmd(RectifierInfo & info);
	bool readProgram(RectifierInfo & info);

	void clearReciveBuffer();
private:
	HANDLE hSerial;
	OVERLAPPED * overlappedRDPtr;
	OVERLAPPED * overlappedWRPtr;
	DWORD * mask;
	std::vector<std::uint8_t> symbolsTail;
	std::map<int, int> registeredRectifiers;
};

class Comport {
public:
private:
	Comport() = delete;
	Comport & operator=(Comport&) = delete;

public:
	static PortState readPort(
		HANDLE hSerial,
		OVERLAPPED * const stateDialogOverlappedRD,
		DWORD * pMask,
		std::vector<std::uint8_t>& rdSymbols);
	static PortState writePort(
		HANDLE hSerial,
		DWORD * pMask,
		OVERLAPPED * const stateDialogOverlappedWR,
		std::vector<std::uint8_t>& rdSymbols);
private:
	static std::vector<std::uint8_t> symbolsTail;
	static bool isValidFrame(std::vector<std::uint8_t> & symbols);

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

struct Reading_thread_param {
	HWND wnd;
	WORD * readingThreadState;
	std::map<int, RectifierInfo> * m_rectifierConfigs;
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


UINT ThreadProc(LPVOID par);
UINT ReadingComPortThread(LPVOID par);

#endif