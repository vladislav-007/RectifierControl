// cncload.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "cncload.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "cncloadDoc.h"
#include "cncloadView.h"
#include "comportdlg.h"
#include "scomport.h"
#include "TaskProgressDlg.h"
#include "StateDialog.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


SThread_param param;
extern WORD threadState;
extern MachineTaskState machinesTasks[];


/////////////////////////////////////////////////////////////////////////////
// CCncloadApp

BEGIN_MESSAGE_MAP(CCncloadApp, CWinApp)
	//{{AFX_MSG_MAP(CCncloadApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_OPTION_COMPORT, OnOptionComport)
	ON_COMMAND(ID_MENU_OPTION_STATESWND, OnMenuOptionStatesWnd)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCncloadApp construction

CCncloadApp::CCncloadApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	portPtr = &port;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCncloadApp object

CCncloadApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCncloadApp initialization

BOOL CCncloadApp::InitInstance()
{

	for(int i=0;i<16;i++){
		machinesTasks[i].taskFileName = NULL;
		machinesTasks[i].taskCFile = NULL;
		machinesTasks[i].bMachineState = 0;
		machinesTasks[i].wNetError = 0;
		machinesTasks[i].wTaskState = 0;
		}

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_CNCLOATYPE,
		RUNTIME_CLASS(CCncloadDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CCncloadView));
	pDocTemplate->SetContainerInfo(IDR_CNCLOATYPE_CNTR_IP);
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	
	

	long res;
	HKEY hk;
	DWORD type, size;
	strcpy( port_name, "COM1" );
	//char name[6]; 
	//strcpy(name, port_name);
	res=RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\cncsoft\\cncloader\\com_dcb",0,KEY_READ, &hk);	
	if( res == ERROR_SUCCESS ){
		//AfxMessageBox("Ветка Software\\cncsoft\\cncloader открылась удачно!");
		//попробуем считать данные настройки компорта
		//size = 16;
		size = sizeof(port_name);
		res = RegQueryValueEx(hk,            // handle to key
			"portname",  // value name	
			NULL,   // reserved
			&type,// type buffer
			(unsigned char *)&port_name,        // data buffer
			&size   // size of data buffer
			);
				
		if( res != ERROR_SUCCESS ){
			AfxMessageBox("параметр portname считан неудачно!");
			LPVOID lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				res,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
			// Free the buffer.
			LocalFree( lpMsgBuf );
			}
		else {
		//	AfxMessageBox( port_name );
			}

		//SetLastError(0);
		size = sizeof(comport_dcb);
		res = RegQueryValueEx(hk,            // handle to key
			"dcb",  // value name	
			0,   // reserved
			&type,// type buffer
			(unsigned char *)&comport_dcb,        // data buffer
			&size // size of data buffer
			);


		if( res != ERROR_SUCCESS ){
			AfxMessageBox("параметр dcb не удалось считать!");
			comport_dcb.DCBlength = 0;
			LPVOID lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				res,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
			// Free the buffer.
			LocalFree( lpMsgBuf );

			}
		else {
			//AfxMessageBox("параметр dcb считался!");
			}
		RegCloseKey( hk );
		}
	else {
		//AfxMessageBox("Ветки Software\\cncsoft\\cncloader\\com_dcb скорее всего нет!Установим настройки по умолчанию.");
		param.statePtr = &wState;
		strcpy( port_name,"COM1" );
		comport_dcb.DCBlength = 0;
		}
	

	comport_dcb.BaudRate = CBR_38400;
	comport_dcb.ByteSize = 8;
	comport_dcb.StopBits = ONESTOPBIT;
	comport_dcb.Parity = NOPARITY;
				

	
	
	port_security_attributes.bInheritHandle = TRUE;
	port_security_attributes.lpSecurityDescriptor = NULL;
	port_security_attributes.nLength = sizeof( port_security_attributes );
		
	//DWORD errCode;
	CString errStr;
	//strcpy(port_name ,  "COM3" );
	//AfxMessageBox( port_name );																					//FILE_FLAG_OVERLAPPED
	port=CreateFile( port_name,GENERIC_READ|GENERIC_WRITE,0,&port_security_attributes,OPEN_EXISTING, NULL, NULL);
	/*errCode = GetLastError();
	errStr.Format("Код ошибки =%d", errCode);
	AfxMessageBox( errStr );
	*/
	//port=CreateFile( (LPCTSTR)ComPortName,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0 ,NULL);
	if(port==INVALID_HANDLE_VALUE) {
		AfxMessageBox("Ошибка открытия последовательного порта");
		setPortState( PORT_CLOSED );
		}
	else {
		CString mStr;
		setPortState( PORT_OPEN );
		if( comport_dcb.DCBlength == 0 ){
			//инициализируем по умолчанию текущими настройками	
			if (!GetCommState(port, &comport_dcb)){
			  // Handle the error.
				mStr.Format("GetCommState failed with error %d.\n", GetLastError());
				// ошибка получения параметров DCB
				//AfxMessageBox("ошибка получения параметров DCB по умолчанию"+mStr);	
				}
			else {
				// структура DCB инициализирована
				comport_dcb.BaudRate = CBR_38400;
				comport_dcb.ByteSize = 8;
				comport_dcb.StopBits = ONESTOPBIT;
				comport_dcb.Parity = NOPARITY;
				//AfxMessageBox("Скорость 38400");	
				}
			}
		
		if(!SetCommState(port, &comport_dcb) ){
			// ошибка установки параметров DCB
			mStr.Format("SetCommState failed with error %d.\n", GetLastError());
			//AfxMessageBox("ошибка установки параметров DCB"+mStr);
			if (!GetCommState(port, &comport_dcb)){
			  // Handle the error.
				mStr.Format("GetCommState failed with error %d.\n", GetLastError());
				// ошибка получения параметров DCB
				AfxMessageBox("ошибка получения параметров DCB по умолчанию"+mStr);	
				}
			else {
				// структура DCB инициализирована
				comport_dcb.BaudRate = CBR_38400;
				comport_dcb.ByteSize = 8;
				comport_dcb.StopBits = ONESTOPBIT;
				comport_dcb.Parity = NOPARITY;
				//AfxMessageBox("Скорость 38400");	
				}
			
			}
		else {
			// параметры DCB установлены
			setPortState( PORT_OPEN | PORT_SETUPED );	
			}
		}




	dwByteTimeOut = 100 + (20/(comport_dcb.BaudRate+1));//таймаут окончания операции чтения или записи байта
	dwWaitTimeOut = dwByteTimeOut;

	COMMTIMEOUTS ct;
	ct.ReadIntervalTimeout= 0;//MAXDWORD;
	ct.ReadTotalTimeoutMultiplier=ct.ReadTotalTimeoutConstant= dwByteTimeOut;
	ct.WriteTotalTimeoutMultiplier=ct.WriteTotalTimeoutConstant=dwByteTimeOut;
//	CString s;
//	s.Format( "Тайм аут = %ld", ct.ReadTotalTimeoutMultiplier );
//	AfxMessageBox( s );

	if( getPortState() ){ 
       	SetCommTimeouts(port,&ct);
        }

	PurgeComm( port , PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR );



	//CSComPort tstComPort;
	//tstComPort.setPortHandle( port );
	//tstComPort.getMachineState( 2  );
       

	param.wnd = pMainFrame->GetSafeHwnd();
	param.portPtr= portPtr;
	param.statePtr = &wState;
	param.portBlock = &portBlock;
	param.dwpByteTimeOut = &dwByteTimeOut;

	//AfxMessageBox( "Запустим поток" );
	AfxBeginThread(ThreadProc, &param,  THREAD_PRIORITY_NORMAL);
	//AfxMessageBox( "Запустли уже поток" );



	stateDlg = new CStateDialog();
	//CMachinesStateWnd();
	RECT rect;
	pMainFrame->GetWindowRect(&rect);
	//pMainFrame->
	stateDlg->Create(IDD_STATE_DIALOG, pMainFrame);
	//stateDlg->Set
	stateDlg->timer = stateDlg->SetTimer( 2, 500, NULL );
	stateDlg->SetWindowPos( pMainFrame, rect.right-120, rect.top+30, 140, 320, 0);
	stateDlg->SetWindowText( "State" );
	//stateDlg->ShowWindow( SW_HIDE );
	pMainFrame->SetFocus();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CCncloadApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CCncloadApp message handlers


void CCncloadApp::OnOptionComport() 
{
	// TODO: Add your command handler code here
		
	CString str;
	CComPortDlg comPortDlg;
	DCB oldDCB;
	
	oldDCB = comport_dcb;
	comPortDlg.m_parity = comport_dcb.Parity;
	comPortDlg.m_port_name.Format( port_name );

	//AfxMessageBox(comPortDlg.m_port_name );
	
	DWORD errCode;
	
	
	str.Format("%ld" , comport_dcb.BaudRate );
	comPortDlg.m_speed = (LPCTSTR)str;
	comPortDlg.m_stop_bits = comport_dcb.StopBits;

	if( ((8 - comport_dcb.ByteSize) >= 0 ) && ((8 - comport_dcb.ByteSize) <= 3 ) ){
		comPortDlg.m_byte_size = (8 - comport_dcb.ByteSize );
		}
	else {
		comPortDlg.m_byte_size = 0;
		}
	
  // Create and show the dialog box
   int nRet = -1;
   nRet = comPortDlg.DoModal();

CString errStr("COM1");

  // Handle the return value from DoModal
	switch ( nRet )
	{
	case -1: 
      AfxMessageBox("Dialog box could not be created!");
      break;
	case IDOK:
      // Do something

		strcpy( port_name, (LPCTSTR)comPortDlg.m_port_name );
		//AfxMessageBox(port_name );

		
		//if( errStr == comPortDlg.m_port_name ){
		//	AfxMessageBox(port_name );
		//	}
	  
	if( ((8 - comPortDlg.m_byte_size) >= 3) && ((8 - comPortDlg.m_byte_size) <= 8) ){
		comport_dcb.ByteSize= (8 - comPortDlg.m_byte_size);
		}
	else {
		comport_dcb.ByteSize = 8;
		}
//LPCTSTR
	
	switch( comPortDlg.m_parity ){
		case 0:
			comport_dcb.Parity = NOPARITY;
			//AfxMessageBox( "NOPARITY");
			break;
		case 1:
			comport_dcb.Parity = ODDPARITY;
			//AfxMessageBox( "ODDPARITY");
			break;
		case 2:
			comport_dcb.Parity = EVENPARITY;
			//AfxMessageBox( "ODDPARITY");
			break;
		case 3:
			comport_dcb.Parity = MARKPARITY;
			//AfxMessageBox( "ODDPARITY");
			break;
		case 4:
			comport_dcb.Parity = SPACEPARITY;
			//AfxMessageBox( "ODDPARITY");
			break;
		default:
			comport_dcb.Parity = NOPARITY;
			//AfxMessageBox( "default NOPARITY");
			break;
		}
	//comport_dcb.Parity = comPortDlg.m_parity;
	if( comPortDlg.m_stop_bits == 1){
		comPortDlg.m_stop_bits = ONE5STOPBITS;
		//str.Format("stop bits=%d", comport_dcb.StopBits );
		//AfxMessageBox( str );
		}
	comport_dcb.StopBits = comPortDlg.m_stop_bits;
	//str.Format("stop bits=%d", comport_dcb.StopBits );
	//AfxMessageBox( str );
	//if( portState.getState( ) ){
		//порт открыт, закроем перед открытием с новыми параметрами
		
		setPortState( 0 );
		Sleep(1);
		//AfxMessageBox( "State set to 0" );
		portBlock.Lock();
		SetLastError(0);
		PurgeComm( port , PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR );
		if( !CloseHandle(port) ){
			AfxMessageBox("Не возможно закрыть последовательный порт");
			errCode = GetLastError();
			errStr.Format("Код ошибки закрытие порта=%d", errCode);
			AfxMessageBox( errStr );
			}

		port_security_attributes.bInheritHandle = TRUE;
		port_security_attributes.lpSecurityDescriptor = NULL;
		port_security_attributes.nLength = sizeof( port_security_attributes );
		SetLastError(0);
		//AfxMessageBox( port_name );
		port=CreateFile( port_name,GENERIC_READ|GENERIC_WRITE,0,&port_security_attributes,OPEN_EXISTING, NULL, NULL );//FILE_FLAG_OVERLAPPED
		errCode = GetLastError();
		portBlock.Unlock();

		if(port==INVALID_HANDLE_VALUE) {
			AfxMessageBox("Не возможно открыть последовательный порт");
			errStr.Format("Код ошибки =%d", errCode);
			AfxMessageBox( errStr );
			}
		else {
			CString mStr;
			//portState.setState( PORT_OPEN );
			if(!SetCommState(port, &comport_dcb) ){
				// ошибка установки параметров DCB
				comport_dcb = oldDCB;
				mStr.Format("SetCommState failed with error %d.\n", GetLastError());
				AfxMessageBox("Некорректные параметры настройки порта"+mStr);
				}
			else {
				// параметры DCB установлены
				//dwByteTimeOut = 2+(20000/comport_dcb.BaudRate+1);//таймаут окончания операции чтения или записи байта
				//dwWaitTimeOut = 2*dwByteTimeOut;
				/*
				COMMTIMEOUTS ct;
				ct.ReadIntervalTimeout= MAXDWORD;
				ct.ReadTotalTimeoutMultiplier=ct.ReadTotalTimeoutConstant= dwByteTimeOut;
				ct.WriteTotalTimeoutMultiplier=ct.WriteTotalTimeoutConstant=dwByteTimeOut;
				if( getPortState() ){ 
       				SetCommTimeouts(port,&ct);
					}

				*/
				PurgeComm( port , PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR );
				}
			setPortState( PORT_OPEN | PORT_SETUPED );
			//AfxMessageBox("Ошибка установки состояния");
			}

		//}

	break;
	case IDCANCEL:
    // Do something
		/*setPortState( 0 );	
		if( CloseHandle(port) == 0 ){
			AfxMessageBox("Ошибка закрытия порта");
			}
		else {
			AfxMessageBox("Порт закрыт");
			}
		port=CreateFile( (LPCTSTR)port_name,GENERIC_READ|GENERIC_WRITE,0,&port_security_attributes,OPEN_EXISTING,FILE_FLAG_OVERLAPPED ,NULL);
		//port = CreateFile( port_name,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED ,NULL);
		setPortState( PORT_OPEN | PORT_SETUPED );	
		*/
    break;
	default:

    // Do something
    break;
	};
	
	//AfxMessageBox( port_name );
	
}


/*
void CCncloadApp::setPort(HANDLE aPort)
{
	portBlock.Lock();
	port = aPort;
	portBlock.Unlock();

}
*/
WORD CCncloadApp::setPortState(WORD aState)
{
	//portBlock.Lock();
	WORD state;
	state = wState;
	wState = aState;
	//portBlock.Unlock();
	return state;
	
}

WORD CCncloadApp::getPortState()
{
	return wState;
}

int CCncloadApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
	HKEY hk;
	DWORD dwDisposition;
	long res;
	DWORD size;

	if( RegCreateKeyEx(HKEY_CURRENT_USER,
		"Software\\cncsoft\\cncloader\\com_dcb",
		0,
		"", //LPTSTR lpClass,
		REG_OPTION_NON_VOLATILE,                            // special options
		KEY_READ|KEY_WRITE,                          // desired security access
		//NULL, // inheritance
		0,
		&hk,
		&dwDisposition                     // disposition value buffer
		) == ERROR_SUCCESS ){   
		
				
		size = sizeof(comport_dcb);						// size of value data
		res =  RegSetValueEx(
			hk,           // handle to key
			"dcb", // value name
			0,      // reserved
			REG_BINARY,        // value type
			(const unsigned char *)&comport_dcb,	// value data
			size);

		if( res != ERROR_SUCCESS ){
			AfxMessageBox("параметр dcb записан неудачно!");
			LPVOID lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
			// Free the buffer.
			LocalFree( lpMsgBuf );
			}
		else {
			//AfxMessageBox("параметр dcb записался!");
			}


	//	CString str;	
	//	str.Format("port number %d", dwPortNumber );
	//	AfxMessageBox(str);

		
		size = sizeof(port_name);
		res = RegSetValueEx(hk,            // handle to key
			"portname",  // value name	
			NULL,   // reserved
			REG_SZ,// type buffer
			(unsigned char *)&port_name,        // data buffer
			size   // size of data buffer
			);
	

		if( res != ERROR_SUCCESS ){
			//AfxMessageBox("параметр dcb записан неудачно!");
			LPVOID lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
			// Free the buffer.
			LocalFree( lpMsgBuf );
			}
		else {
			//AfxMessageBox("параметр dcb записался!");
			}

		RegCloseKey( hk );
		}
	else {
		AfxMessageBox("Ошибка сохранения параметров в реестре!");
		}

	setPortState( 0 );
	//AfxMessageBox("остановим поток");
	if(threadState == 1){
		threadState = 2;
		}
	while( threadState == 2)Sleep(1);
	if(threadState == 3){
		//AfxMessageBox("Поток остановлен");
		}

	
	CloseHandle(port);
	
	
	/*
	if (m_bATLInited)
	{
		_Module.RevokeClassObjects();
		_Module.Term();
	
		CoUninitialize();
	}
	*/
	/*for(int i=0;i<15;i++){
		delete machinesTasks[i].taskFileName;
		delete machinesTasks[i].taskCFile;
		}
	*/
	return CWinApp::ExitInstance();
}



CDocument* CCncloadApp::OpenDocumentFile(LPCTSTR lpszFileName) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWinApp::OpenDocumentFile(lpszFileName);
}

void CCncloadApp::OnMenuOptionStatesWnd() 
{
	stateDlg->ShowWindow( SW_SHOW );
}

#define TEST

long CCncloadApp::loadCNCProgram(LPCSTR fileName, BYTE machineNumber)
{
	
	CStdioFile * taskFile;

	
	#ifdef TEST
	CStdioFile * testFile;
	CString *testFileName;
	#endif

	CStdioFile * testTxtFile;
	CString *testTxtFileName;


	CStdioFile sourceFile;

	static BYTE number = 0;
	CString * cncFileName, aStr;

	


	if(	machinesTasks[machineNumber].wTaskState&(TASK_SET|TASK_STARTED) ){
		//machinesTasks[machineNumber].taskCFile = taskFile;
		AfxMessageBox("На указанный станок программа уже загружается!");
		return machinesTasks[machineNumber].wTaskState;
		}

	number ++;

	cncFileName = new CString();
	cncFileName->Format("cnc%d.dpd", number );
	taskFile = new CStdioFile(); 

#ifdef TEST
	testFileName = new CString();	
	testFileName->Format("cnc%d.tst", number );
	testFile = new CStdioFile(); 
#endif

	testTxtFileName = new CString();	
	testTxtFileName->Format("cnc%d.txt", number );
	testTxtFile = new CStdioFile(); 



	char ch0A; 
	char ch0D; 
	ch0A = 0x0A;
	ch0D = 0x0D;

	
	

	if( !sourceFile.Open(fileName, CFile::modeRead|CFile::typeText ) ){
		AfxMessageBox("Не удалось открыть файл источник");
		}
	else {

		//while (access(lpszFileName, 0) != -1)
		//	MessageBox("not found");
		//	}



		while( taskFile->Open( (LPCTSTR) *cncFileName, CFile::modeRead|CFile::typeText) ){
			//(*cncFileName)+="Удалось открыть файл программы, это плохо";
			taskFile->Close();
			//AfxMessageBox( *cncFileName );
			number ++;
			cncFileName->Format("cnc%d.dpd", number );
			}

		if( taskFile->Open( (LPCTSTR) (*cncFileName), CFile::modeReadWrite | CFile::modeCreate|CFile::typeBinary) ){
			//aStr ="Удалось открыть файл программы, новый";
			//aStr+= taskFile->GetFilePath();
			//AfxMessageBox( aStr );
			//BYTE byte[512];
			//UINT nCount; 
			CString rStr, nextStr;
			//сичтаем по строчно
			sourceFile.SeekToBegin();
			taskFile->SeekToBegin();
			#ifdef TEST
			testFile->Open( (LPCTSTR) (*testFileName), CFile::modeReadWrite | CFile::modeCreate|CFile::typeBinary);//CFile::typeText);
			#endif

			testTxtFile->Open( (LPCTSTR) (*testTxtFileName), CFile::modeReadWrite | CFile::modeCreate|CFile::typeBinary);//CFile::typeText);
						
			//пропустим шапку
			while( sourceFile.ReadString( rStr ) ){
				rStr.MakeUpper();
				if( (rStr.Find( "%" ) == -1 ) ){//&& (rStr.Find( "L" ) == -1) ){
					//не найдены такие символы
					}
				else {
					//считали символ % 
					break;
					}
				}
			
			sourceFile.ReadString( nextStr );
			nextStr.MakeUpper();
			if( (nextStr.Find( "L" ) == -1 ) ){
				//не найден такой символ
				//rStr.Replace( 0x0D, 0x0A);
				taskFile->WriteString( (LPCTSTR)rStr );
				//taskFile->Write( &ch0D, 1 );
				taskFile->Write( &ch0A, 1 );
#ifdef TEST
				testFile->WriteString( (LPCTSTR)rStr );
				//taskFile->Write( &ch0D, 1 );
				testFile->Write( &ch0A, 1 );
#endif
				}
			else {
				//считали символ L
				//следовательно % не учитываем
				}

			//if( nextStr.Replace( 0x0D, 0x0A)== 0 ){
			//	AfxMessageBox( "не найден символ 0x0d" );
			//	}
			//;
			taskFile->WriteString( (LPCTSTR)nextStr );
			taskFile->Write( &ch0A, 1 );
			//taskFile->Write( &ch0D, 1 );
			//тестовый файл
			#ifdef TEST
			testFile->WriteString( (LPCTSTR)nextStr );
			testFile->Write( &ch0A, 1 );
			#endif

			
		
			while( sourceFile.ReadString( nextStr ) ){
				//nextStr.Replace( 0x0D, 0x0A);
				taskFile->WriteString( (LPCTSTR)nextStr );
				taskFile->Write( &ch0A, 1 );
				#ifdef TEST
				testFile->WriteString( (LPCTSTR)nextStr );
				testFile->Write( &ch0A, 1 );
				#endif
				}


			BYTE byte[516];
			UINT nCount;
			taskFile->Seek(0,0);
			while( (nCount = taskFile->Read( byte, 512 )) ){
				//сделаем байты четными 
				unsigned char ch;
				for( UINT j=0; j < nCount;j++){
					ch = byte[j];
					for(int i=4;i>=1;i=(i>>1)){
						ch^=ch<<i;
						}
					byte[j]= byte[j]^(ch&0x80);
					}

				testTxtFile->Write( byte, nCount );
				}

			testTxtFile->Seek(0,0);

			taskFile->Seek(0,0);
			//taskFile->SetLength( 0 );

			testFile->Seek(0,0);
			//testFile->SetLength( 0 );

			while( (nCount = testTxtFile->Read( byte, 512 )) ){
				//сделаем байты четными 
				taskFile->Write( byte, nCount );
				testFile->Write( byte, nCount );
				}
			

			#ifdef TEST
			testFile->Close();
			#endif	


			testTxtFile->Close();


			taskFile->Seek(0,0);
			machinesTasks[machineNumber].taskFileName = cncFileName;
			machinesTasks[machineNumber].taskCFile = taskFile;
			machinesTasks[machineNumber].wTaskState = TASK_SET;
			// 27.07.09 add init befor load
			machinesTasks[machineNumber].shouldInitByFF = true;

			//AfxMessageBox( machinesTasks[machineNumber].taskCFile->GetFilePath() );
			
			CTaskProgressDlg * taskDlg;
			taskDlg = new CTaskProgressDlg();
			taskDlg->machineNumber = machineNumber;
			//CDialog
			CString title;

			
			taskDlg->m_machine_state.Format("Состояние станка № %d", machineNumber );
			taskDlg->Create( IDD_DIALOG2);//, pMainFrame );
			//taskDlg->GetWindowText(title);
			//AfxMessageBox( title );
			title.Format("%d Загрузка программы в станок № %d", machineNumber, machineNumber );
			taskDlg->SetWindowText( (LPCTSTR)title );
			
			
			taskDlg->timer = taskDlg->SetTimer( 1, 500, NULL );

			DWORD dwLength;
			dwLength = taskFile->GetLength();
			taskDlg->m_progress_bar.SetRange(0,(short)dwLength);
			//taskDlg->GetWindowText(title);
			//taskDlg->
			}
		sourceFile.Close();
		}
	//delete taskFile;
	return 0;
	}


