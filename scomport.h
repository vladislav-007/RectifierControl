#ifndef _SCOM_PORT
#define _SCOM_PORT
#include "stdafx.h"
#include <map>
#include <afxmt.h>

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
	int id;
	CString name;
	int address;
	CString comport;
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

//typedef struct tagNMHDROBJECT
//{
//	NMHDR nmHdr;
//	CNotifyObject * pObject;
//} NMHDROBJECT;
//
//typedef NMHDROBJECT * LPNMHDROBJECT;

struct SThread_param {
	HWND wnd;
	//HANDLE * portPtr;
	WORD * statePtr;
	//CCriticalSection * portBlock;
	//DWORD * dwpByteTimeOut;
	std::map<int, RectifierInfo> * m_rectifierConfigs;
	//OVERLAPPED * Sync;
};



struct MachineTaskState {
	WORD wTaskState;
	CString * taskFileName;
	CStdioFile * taskCFile;
	BYTE bMachineState;
	WORD wNetError;
	bool shouldInitByFF;
	};

class CSComPort{
public:
	CSComPort();
	//CSComPort( HANDLE port );
	//~CSComPort();
	void setPortHandle( HANDLE port ){
		portHandle = port;
		}
	HANDLE getPortHandle(){
		return portHandle;
		}
	void setWnd( HWND aWnd){
		wnd = aWnd;
		}
	WORD makeDataBurst( BYTE address, BYTE cmd, const BYTE * data, BYTE length , BYTE burst[136] );
	WORD parseMachineBurst( const BYTE * burst, BYTE length, BYTE & address, BYTE * data );
	BYTE getMachineState( BYTE address );
	BYTE setMachineState( BYTE address, BYTE state=0 );
	BYTE writeDataToMachine( const BYTE address, const BYTE * aData, const UINT aLength );
	WORD readPort( BYTE * buff, DWORD size, BYTE endByte = STOP_BYTE );
	//BYTE getMachineState( BYTE address );
	void clearError(){
		error = 0;
		}
	WORD getError();
private:
	HANDLE portHandle;
	HWND wnd;
	DWORD dwByteTimeOut; 
	WORD error;
};

UINT ThreadProc(LPVOID par );

#endif