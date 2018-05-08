
// RectifierControl.cpp : Определяет поведение классов для приложения.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "RectifierControl.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "RectifierControlDoc.h"
#include "RectifierControlView.h"

#include "SetComportDlg.h"
#include "tinyxml2.h"
#include <filesystem>
#include <cstdint>

namespace fs = std::experimental::filesystem;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRectifierControlApp

BEGIN_MESSAGE_MAP(CRectifierControlApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CRectifierControlApp::OnAppAbout)
	// Стандартные команды по работе с файлами документов
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// Стандартная команда настройки печати
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_LINK_OPTIONS, &CRectifierControlApp::OnLinkOptions)
END_MESSAGE_MAP()


// создание CRectifierControlApp

CRectifierControlApp::CRectifierControlApp()
{
	// поддержка диспетчера перезагрузки
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// Если приложение построено с поддержкой среды Common Language Runtime (/clr):
	//     1) Этот дополнительный параметр требуется для правильной поддержки работы диспетчера перезагрузки.
	//   2) В своем проекте для сборки необходимо добавить ссылку на System.Windows.Forms.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: замените ниже строку идентификатора приложения строкой уникального идентификатора; рекомендуемый
	// формат для строки: ИмяКомпании.ИмяПродукта.СубПродукт.СведенияОВерсии
	SetAppID(_T("RectifierControl.AppID.NoVersion"));

	// TODO: добавьте код создания,
	// Размещает весь важный код инициализации в InitInstance
	m_usedComPort = _T("");
}

// Единственный объект CRectifierControlApp

CRectifierControlApp theApp;

enum class Parity : std::int8_t {
	NO_PARITY = 0,
	ODD_PARITY = 1,
	EVEN_PARITY = 2,
	MARK_PARITY = 3,
	SPACE_PARITY = 4
};

CString toString(Parity parity) {
	switch (parity) {
	case Parity::NO_PARITY:
		return CString("NOPARITY");
	case Parity::EVEN_PARITY:
		return CString("EVENPARITY");
	case Parity::MARK_PARITY:
		return CString("MARKPARITY");
	case Parity::ODD_PARITY:
		return CString("ODDPARITY");
	case Parity::SPACE_PARITY:
		return CString("SPACEPARITY");
	}
}

Parity parityFromString(const CString & parityStr) {
	if (0 == parityStr.CompareNoCase(L"NOPARITY"))
		return Parity::NO_PARITY;
	if (0 == parityStr.CompareNoCase(L"EVENPARITY"))
		return Parity::EVEN_PARITY;
	if (0 == parityStr.CompareNoCase(L"MARKPARITY"))
		return Parity::MARK_PARITY;
	if (0 == parityStr.CompareNoCase(L"ODDPARITY"))
		return Parity::ODD_PARITY;
	if (0 == parityStr.CompareNoCase(L"SPACEPARITY"))
		return Parity::SPACE_PARITY;


	CT2A ascii(parityStr, CP_UTF8);
	throw std::invalid_argument(ascii.m_psz);
}

enum class Stopbits : std::int8_t {
	ONE_STOPBIT = 0,
	ONE5_STOPBITS = 1,
	TWO_STOPBITS = 2
};

CString toString(Stopbits stopBits) {
	switch (stopBits) {
	case Stopbits::ONE_STOPBIT:
		return CString("ONESTOPBIT");
	case Stopbits::ONE5_STOPBITS:
		return CString("ONE5STOPBIT");
	case Stopbits::TWO_STOPBITS:
		return CString("TWOSTOPBITS");
	}
}

Stopbits stopbitsFromString(const CString & stopBitsStr) {
	if (0 == stopBitsStr.CompareNoCase(L"ONESTOPBIT"))
		return Stopbits::ONE_STOPBIT;
	if (0 == stopBitsStr.CompareNoCase(L"ONE5STOPBIT"))
		return Stopbits::ONE5_STOPBITS;
	if (0 == stopBitsStr.CompareNoCase(L"TWOSTOPBITS"))
		return Stopbits::TWO_STOPBITS;
	CT2A ascii(stopBitsStr, CP_UTF8);
	throw std::invalid_argument(ascii.m_psz);
}


struct RectifierInfo {
	int id;
	CString name;
	int address;
	CString comport;
	int modeID;
	CString modeName;
	int modeBoundRate;
	int byteSize;
	Parity parity;
	Stopbits stopbits;


};

// инициализация CRectifierControlApp

BOOL CRectifierControlApp::InitInstance()
{
	// InitCommonControlsEx() требуются для Windows XP, если манифест
	// приложения использует ComCtl32.dll версии 6 или более поздней версии для включения
	// стилей отображения.  В противном случае будет возникать сбой при создании любого окна.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Выберите этот параметр для включения всех общих классов управления, которые необходимо использовать
	// в вашем приложении.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Инициализация библиотек OLE
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// Для использования элемента управления RichEdit требуется метод AfxInitRichEdit2()	
	// AfxInitRichEdit2();

	// Стандартная инициализация
	// Если эти возможности не используются и необходимо уменьшить размер
	// конечного исполняемого файла, необходимо удалить из следующего
	// конкретные процедуры инициализации, которые не требуются
	// Измените раздел реестра, в котором хранятся параметры
	// TODO: следует изменить эту строку на что-нибудь подходящее,
	// например на название организации
	SetRegistryKey(_T("Локальные приложения, созданные с помощью мастера приложений"));
	LoadStdProfileSettings(4);  // Загрузите стандартные параметры INI-файла (включая MRU)

	// set default comport  options, in future get it from saved config files
	COMMCONFIG commconfig;
	commconfig.dcb.BaudRate = CBR_115200;
	commconfig.dcb.ByteSize = 7;
	commconfig.dcb.Parity = NOPARITY;
	commconfig.dcb.StopBits = ONESTOPBIT;
	m_comportProperties[CString("DefaultPort")] = commconfig;

	//try to find config file
	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		DWORD error = GetLastError();
	}
	
	fs::path dir(szPath);
	fs::path file("RectifierControlConfig.xml");
	fs::path full_path = dir.parent_path() / file;
	std::string s = full_path.string(); 
	const char* filePath = s.c_str();
	tinyxml2::XMLDocument doc;
	doc.LoadFile(filePath);
	if (doc.Error()) {
		const char * msg = doc.ErrorStr();
		DWORD error = GetLastError();
	}
	else {
		tinyxml2::XMLElement * rectifiers = doc.FirstChildElement("RectifierController")->FirstChildElement("Rectifiers");
		tinyxml2::XMLElement * rectifier = rectifiers->FirstChildElement("Rectifier");
		while (rectifier != nullptr) {
			RectifierInfo rectifierInfo;
			if (tinyxml2::XML_SUCCESS != rectifier->QueryIntAttribute("id", &rectifierInfo.id))
				throw std::exception("Can't read 'id' if rectifier");
			const char * str;
			rectifier->QueryStringAttribute("name", &str);
			rectifierInfo.name = CA2T(str, CP_UTF8);
			rectifier->QueryStringAttribute("comport", &str);
			rectifierInfo.comport = CA2T(str, CP_UTF8);
			rectifier->QueryIntAttribute("address", &rectifierInfo.address);
			tinyxml2::XMLElement * mode = rectifier->FirstChildElement("Mode");
			if (tinyxml2::XML_SUCCESS != mode->QueryIntAttribute("id", &rectifierInfo.id))
				throw std::exception("Can't read 'id' if rectifier's mode");
			
			mode->QueryStringAttribute("name", &str);
			rectifierInfo.modeName = CA2T(str, CP_UTF8);
			tinyxml2::XMLElement * valueElement = mode->FirstChildElement("BaudRate");
			rectifierInfo.modeBoundRate = valueElement->QueryIntAttribute("value", &rectifierInfo.address);
			valueElement = mode->FirstChildElement("ByteSize");
			valueElement->QueryIntAttribute("value", &rectifierInfo.byteSize);
			valueElement = mode->FirstChildElement("Parity");
			valueElement->QueryStringAttribute("value", &str);
			CString valueStr = CA2T(str, CP_UTF8);
			rectifierInfo.parity = parityFromString(valueStr);
			valueElement = mode->FirstChildElement("StopBits");
			valueElement->QueryStringAttribute("value", &str);
			valueStr = CA2T(str, CP_UTF8);
			rectifierInfo.parity = parityFromString(valueStr);
			
			/*<Parity value = "NOPARITY" / >
			<StopBits value = "ONESTOPBIT" / >*/

			rectifier->QueryIntAttribute("address", &rectifierInfo.address);




			MessageBox(NULL, rectifierInfo.name, _T("message"), MB_OK | MB_SYSTEMMODAL);


			rectifier = rectifier->NextSiblingElement();
		}
	}



	// Зарегистрируйте шаблоны документов приложения.  Шаблоны документов
	//  выступают в роли посредника между документами, окнами рамок и представлениями
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_RectifierControTYPE,
		RUNTIME_CLASS(CRectifierControlDoc),
		RUNTIME_CLASS(CChildFrame), // настраиваемая дочерняя рамка MDI
		RUNTIME_CLASS(CRectifierControlView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// создайте главное окно рамки MDI
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	// вызов DragAcceptFiles только при наличии суффикса
	//  В приложении MDI это должно произойти сразу после задания m_pMainWnd
	// Включить открытие перетаскивания
	m_pMainWnd->DragAcceptFiles();

	// Разрешить использование расширенных символов в горячих клавишах меню
	CMFCToolBar::m_bExtCharTranslation = TRUE;

	// Синтаксический разбор командной строки на стандартные команды оболочки, DDE, открытие файлов
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)   // actually none
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Включить открытие выполнения DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Команды диспетчеризации, указанные в командной строке.  Значение FALSE будет возвращено, если
	// приложение было запущено с параметром /RegServer, /Register, /Unregserver или /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// Главное окно было инициализировано, поэтому отобразите и обновите его
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CRectifierControlApp::ExitInstance()
{
	//TODO: обработайте дополнительные ресурсы, которые могли быть добавлены
	AfxOleTerm(FALSE);

	return CWinApp::ExitInstance();
}

// обработчики сообщений CRectifierControlApp


// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

// Реализация
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// Команда приложения для запуска диалога
void CRectifierControlApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// обработчики сообщений CRectifierControlApp




bool verifyCommOptions(const COMMCONFIG & comm)
{
	if (comm.dcb.BaudRate > 115200)
		return false;

}

void CRectifierControlApp::OnLinkOptions()
{
	// TODO: Add your command handler code here
	CSetComportDlg setComPortDlg;
	setComPortDlg.setCurrentlyUsedComport(m_usedComPort);
	INT_PTR res = setComPortDlg.DoModal();
	if (IDOK == res) {
		
		HWND hWnd = AfxGetMainWnd()->m_hWnd;
		HWND hwnd = GetDlgItem(setComPortDlg, IDC_COMBO_COMPORT);
		CComboBox * cmbBox_ComPort = (CComboBox*)(hwnd);
		CString selectedPort = setComPortDlg.getComport();
		m_usedComPort = selectedPort;
		//		cmbBox_ComPort->GetLBText(cmbBox_ComPort->GetCurSel(), selectedPort);
		COMMCONFIG comm = m_comportProperties[CString("DefaultPort")];
		CommConfigDialog(selectedPort, hWnd, &comm);
		while (!verifyCommOptions(comm)) {
			CommConfigDialog(selectedPort, hWnd, &comm);
		}
	}
}
