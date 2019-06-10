
// RectifierControlView.cpp : реализация класса CRectifierControlView
//

#include "stdafx.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позволяет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "RectifierControl.h"
#endif

#include "RectifierControlDoc.h"
#include "RectifierControlView.h"
#include "SetParametersDialog.h"

#include <sstream>
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRectifierControlView

IMPLEMENT_DYNCREATE(CRectifierControlView, CView)

BEGIN_MESSAGE_MAP(CRectifierControlView, CView)
	// Стандартные команды печати
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_COMMAND(ID_OPEN_SET_PARAMETERS_DIALOG, &CRectifierControlView::OnOpenSetParametersDialog)
	ON_COMMAND(ID_START_PROGRAM, &CRectifierControlView::OnStartProgram)
	ON_COMMAND(ID_STOP_PROGRAM, &CRectifierControlView::OnStopProgram)
	//	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// создание/уничтожение CRectifierControlView
HICON CRectifierControlView::aIcons[MAXICONS];

CRectifierControlView::CRectifierControlView()
{
	// TODO: добавьте код создания
	//CFormView()
	mp_Font = new CFont();
	mp_Font->CreateFont(28, 0, 0, 0, 600, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	m_fontHeight = 32;
	m_normalFont = new CFont();
	m_normalFont->CreateFont(m_fontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	m_Voltage = 0.0;
	m_Current = 0.0;
	m_VoltageToSet = 0.0;
	m_CurrentToSet = 0.0;
	aIcons[0] = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_STATUS_OK));
	aIcons[1] = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_STATUS_PAUSE));
	aIcons[2] = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_STATUS_RUN));
	aIcons[3] = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_STATUS_NO));
}

CRectifierControlView::~CRectifierControlView()
{
	delete mp_Font;
}

BOOL CRectifierControlView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: изменить класс Window или стили посредством изменения
	//  CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// рисование CRectifierControlView

const CString STATE_UNDEFINED = CString(CA2T("UNDEFINED", CP_UTF8));
const CString STATE_OK = CString(CA2T("OK", CP_UTF8));
const CString STATE_FAILED_TO_GET_STATE_F07 = CString(CA2T("Не удалось получить состояние выпрямителя", CP_UTF8));
const CString STATE_INVALID_USER_BUFFER = CString(CA2T("Ошибка работы программы (невалидный буфер)", CP_UTF8));
const CString STATE_UNKNOWN_ERROR = CString(CA2T("Неизвестная ошибка", CP_UTF8));
const CString STATE_FAILED_TO_OPEN_COMPORT = CString(CA2T("Ошибка открытия COM порта", CP_UTF8));
const CString STATE_NOT_INITIALIZED = CString(CA2T("Запрос состояния выпрямителя", CP_UTF8));
const CString STATE_DEVICE_IS_NOT_READY = CString(CA2T("Выпрямитель не готов к работе", CP_UTF8));
const CString STATE_ADDRESS_DOESNT_MATCH = CString(CA2T("Неверный адрес устройства (готовность)", CP_UTF8));
const CString STATE_DATA_ADDRESS_DOESNT_MATCH = CString(CA2T("Неверный адрес устройства (данные)", CP_UTF8));

const CString & toString(RectifierState state) {
	switch (state) {
	default:
		return STATE_UNDEFINED;
	case RectifierState::OK:
		return STATE_OK;
	case RectifierState::FAILED_TO_GET_STATE:
		return STATE_FAILED_TO_GET_STATE_F07;
	case RectifierState::INVALID_USER_BUFFER:
		return STATE_INVALID_USER_BUFFER;
	case RectifierState::UNKNOWN_ERROR:
		return STATE_UNKNOWN_ERROR;
	case RectifierState::FAILED_TO_OPEN_COMPORT:
		return STATE_FAILED_TO_OPEN_COMPORT;
	case RectifierState::NOT_INITIALIZED:
		return STATE_NOT_INITIALIZED;
	case RectifierState::DEVICE_ISNT_READY:
		return STATE_DEVICE_IS_NOT_READY;
	case RectifierState::ADDRESS_DOESNT_MATCH:
		return STATE_ADDRESS_DOESNT_MATCH;
	case RectifierState::DATA_ADDRESS_DOESNT_MATCH:
		return STATE_DATA_ADDRESS_DOESNT_MATCH;
	}
}


void DrawEllipse(HWND hwnd, HDC hdc, _In_ int left, _In_ int top, _In_ int right, _In_ int bottom, COLORREF color) /*<-- dc sent to this function now*/
{
	/*create and select gdi brush*/
	HBRUSH hbr = CreateSolidBrush(color);
	HBRUSH hOld = (HBRUSH)SelectObject(hdc, hbr);
	/*draw ellipse*/
	Ellipse(hdc, left, top, right, bottom);
	/*restore device context's original, default brush object*/
	SelectObject(hdc, hOld);
	/*free brush resources back to system*/
	DeleteObject(hbr);
}

void CRectifierControlView::DocToClient(CRect& rect)
{
	CClientDC dc(this);
	OnPrepareDC(&dc, NULL);
	dc.LPtoDP(rect);
	rect.NormalizeRect();
}

void CRectifierControlView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnPrepareDC(pDC, pInfo);

	// mapping mode is MM_ANISOTROPIC
	// these extents setup a mode similar to MM_LOENGLISH
	// MM_LOENGLISH is in .01 physical inches
	// these extents provide .01 logical inches

	pDC->SetMapMode(MM_ANISOTROPIC);

	int res = pDC->GetDeviceCaps(DRIVERVERSION);

	res = pDC->GetDeviceCaps(LOGPIXELSX);

	pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX),	pDC->GetDeviceCaps(LOGPIXELSY));
	pDC->SetWindowExt(100, -100);

	//// set the origin of the coordinate system to the center of the page
	//CPoint ptOrg;
	//ptOrg.x = GetDocument()->GetSize().cx / 2;
	//ptOrg.y = GetDocument()->GetSize().cy / 2;

	//// ptOrg is in logical coordinates
	//pDC->OffsetWindowOrg(-ptOrg.x, ptOrg.y);
}

void CRectifierControlView::OnDraw(CDC* pDC)
{

	CRectifierControlDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// only paint the rect that needs repainting
	CRect client;
	pDC->GetClipBox(client);
	CRect rect = client;
	DocToClient(rect);

	CDC dc;
	CDC* pDrawDC = pDC;
	CBitmap bitmap;
	CBitmap* pOldBitmap = 0;

	if (!pDC->IsPrinting())
	{
		// draw to offscreen bitmap for fast looking repaints
		if (dc.CreateCompatibleDC(pDC))
		{
			if (bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height()))
			{
				OnPrepareDC(&dc, NULL);
				pDrawDC = &dc;

				// offset origin more because bitmap is just piece of the whole drawing
				dc.OffsetViewportOrg(-rect.left, -rect.top);
				pOldBitmap = dc.SelectObject(&bitmap);
				dc.SetBrushOrg(rect.left % 8, rect.top % 8);

				// might as well clip to the same rectangle
				dc.IntersectClipRect(client);
			}
		}
	}

	//// paint background
	//CBrush brush;
	//if (!brush.CreateSolidBrush(pDoc->GetPaperColor()))
	//	return;

	//brush.UnrealizeObject();
	//pDrawDC->FillRect(client, &brush);

	//if (!pDC->IsPrinting() && m_bGrid)
	//	DrawGrid(pDrawDC);

	//pDoc->Draw(pDrawDC, this);

	if (pDrawDC != pDC)
	{
		pDC->SetViewportOrg(0, 0);
		pDC->SetWindowOrg(0, 0);
		pDC->SetMapMode(MM_TEXT);
		dc.SetViewportOrg(0, 0);
		dc.SetWindowOrg(0, 0);
		dc.SetMapMode(MM_TEXT);
		pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(),
			&dc, 0, 0, SRCCOPY);
		dc.SelectObject(pOldBitmap);
	}


	CString rectifierName(CA2T("", CP_UTF8));


	pDC->SelectObject(m_normalFont);
	pDC->SetTextColor(RGB(0, 0, 0));
	int rectifierID = pDoc->getRectifierInfo().id;
	rectifierName += pDoc->getRectifierInfo().name;
	rectifierName += " (";
	CString strAddr;
	strAddr.Format(L"%d", pDoc->getRectifierInfo().address);
	rectifierName += strAddr;
	rectifierName += ", ";
	rectifierName += pDoc->getRectifierInfo().comport;
	rectifierName += ")";
	std::map<int, RectifierInfo> & actualRectifiesInfos = theApp.getRectifierInfos();

	CString rectifierState(CA2T("Состояние: ", CP_UTF8));
	std::wstringstream ss;
	const RectifierInfo & info = actualRectifiesInfos.at(rectifierID);

	// Multythreading Synchronization
	CSingleLock lock(&info.cs, true);

	const CString & strState = toString(info.state);
	CString deviceState(strState);
	bool overHeatFlag = false;
	bool protectionFlag = false;
	std::wstringstream overHeatChannels;
	std::wstringstream protectionChannels;
	for (int k = 1; k <= 4; ++k) {
		uint8_t chanState = info.stateF05.getChan(k);
		if (chanState & 0x02) {
			overHeatChannels << k;
			if (k > 1) {
				overHeatChannels << ",";
			}
			overHeatFlag = true;
		}
		if (chanState & 0x04) {
			protectionChannels << k;
			if (k > 1) {
				protectionChannels << ",";
			}
			protectionFlag = true;
		}

	}

	rectifierState += "(";
	ss << std::setw(2);
	ss << L"0x" << std::setfill(L'0') << std::setw(2) << std::hex << info.stateF05.getChan(1) << L",";
	ss << L"0x" << std::setfill(L'0') << std::setw(2) << std::hex << info.stateF05.getControlByte();
	CString stateBytes;
	stateBytes = ss.str().c_str();
	rectifierState += stateBytes;
	rectifierState += ") ";

	if ((info.stateF05.getControlByte() & 0x04) == 0x04) {
		// program is currently executed
		GetDlgItem(ID_START_PROGRAM)->EnableWindow(FALSE);
		GetDlgItem(ID_STOP_PROGRAM)->EnableWindow(TRUE);
	}
	else {
		GetDlgItem(ID_START_PROGRAM)->EnableWindow(TRUE);
		GetDlgItem(ID_STOP_PROGRAM)->EnableWindow(FALSE);
	}

	CString overheatMsg(CA2T("Перегрев", CP_UTF8));
	CString protectMsg(CA2T("Защита", CP_UTF8));
	COLORREF overheatColor = RGB(250, 0, 100);
	COLORREF protectColor = RGB(230, 100, 100);
	COLORREF whiteColor = RGB(254, 254, 254);
	// color indicator
	//circle.Height = 20; //or some size
	//circle.Width = 20; //height and width is the same for a circle
	//circle.Fill = System.Windows.Media.Brushes.Red;

	if (info.state == RectifierState::OK) {
		if (overHeatFlag) {
			RECT rect;
			::GetClientRect(m_hWnd, &rect);
			pDC->FillRect(&rect, &CBrush(overheatColor));
			pDC->SetBkColor(overheatColor);
			rectifierState += overheatMsg;
		}
		else {
			if (protectionFlag) {
				RECT rect;
				::GetClientRect(m_hWnd, &rect);
				pDC->FillRect(&rect, &CBrush(protectColor));
				pDC->SetBkColor(protectColor);
				rectifierState += protectMsg;
			}
			else {
				rectifierState += deviceState;
				RECT rect;
				::GetClientRect(m_hWnd, &rect);
				pDC->FillRect(&rect, &CBrush(whiteColor));
				pDC->SetBkColor(whiteColor);
			}
		}
	}
	else {

		HBRUSH brush = ((HBRUSH)::GetStockObject(LTGRAY_BRUSH));
		pDC->SelectObject(brush);
		RECT rect;
		::GetClientRect(m_hWnd, &rect);
		pDC->Rectangle(&rect);

		LOGBRUSH logBrush;
		GetObject(brush, sizeof(LOGBRUSH), &logBrush);
		COLORREF color = logBrush.lbColor;
		pDC->SetBkColor(color);
		rectifierState += deviceState;
	}

	pDC->TextOutW(10, 10, rectifierName);
	pDC->TextOutW(10, 10 + m_fontHeight, rectifierState);

	CFont headerFont;
	headerFont.CreateFont(2 * m_fontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");

	pDC->SelectObject(headerFont);

	int yShift = 25;

	CString voltageValue(CA2T("V: ", CP_UTF8));
	pDC->TextOutW(10, yShift + 10 + 2 * m_fontHeight, voltageValue);
	ss.str(std::wstring());
	if (info.state != RectifierState::OK) {
		ss << " -- ";
	}
	else {
		ss.setf(std::ios::floatfield, std::ios::fixed);
		m_Voltage = ((float)info.stateF05.getV()) / 10.0f;
		ss << std::setprecision(1) << m_Voltage;
	}
	voltageValue = ss.str().c_str();
	CFont valuesFont;
	valuesFont.CreateFont(2 * m_fontHeight, 0, 0, 0, 600, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	pDC->SelectObject(valuesFont);
	pDC->SetTextColor(RGB(0, 0, 255));
	pDC->TextOutW(80, yShift + 10 + 2 * m_fontHeight, voltageValue);

	pDC->SelectObject(headerFont);
	pDC->SetTextColor(RGB(0, 0, 0));
	CString currentValue(CA2T("A: ", CP_UTF8));
	pDC->TextOutW(10, yShift + 10 + 4 * m_fontHeight, currentValue);
	ss.str(std::wstring());
	ss << std::dec;
	if (info.state != RectifierState::OK) {
		ss << " -- ";
	}
	else {
		ss.setf(std::ios::floatfield, std::ios::fixed);
		m_Current = ((float)(info.stateF05.getHiA() * 255 + info.stateF05.getLowA()));
		ss << std::setprecision(1) << m_Current;
	}

	currentValue = ss.str().c_str();
	pDC->SelectObject(valuesFont);
	pDC->SetTextColor(RGB(0, 255, 0));
	pDC->TextOutW(80, yShift + 10 + 4 * m_fontHeight, currentValue);

	//=================================
	pDC->SelectObject(headerFont);
	pDC->SetTextColor(RGB(0, 0, 0));

	currentValue = CString(CA2T("T: ", CP_UTF8));
	pDC->TextOutW(10, yShift + 10 + 6 * m_fontHeight, currentValue);
	ss.str(std::wstring());
	if (info.state != RectifierState::OK) {
		ss << "--:--:--";
	}
	else {
		ss << std::setfill(L'0') << std::setw(2) << info.stateF05.getHours();
		ss << ":";
		ss << std::setfill(L'0') << std::setw(2) << info.stateF05.getMinutes();
		ss << ":";
		ss << std::setfill(L'0') << std::setw(2) << info.stateF05.getSeconds();
	}
	currentValue = ss.str().c_str();
	pDC->SelectObject(valuesFont);
	pDC->SetTextColor(RGB(0, 255, 0));
	pDC->TextOutW(80, yShift + 10 + 6 * m_fontHeight, currentValue);

	//pDC->SelectObject(m_normalFont);
	//if (info.cmdToSend.rectifierCmd == RectifierCmd::SET_VOLTAGE_AND_CURRENT
	//	&& CmdStatus::SUCCEEDED == info.cmdToSend.status) {
	//	m_VoltageToSet = info.cmdToSend.voltage;
	//	m_CurrentToSet = info.cmdToSend.current;
	//}

	if (info.memory.size() > 7) {
		m_CurrentToSet = (float)(info.memory[1] + info.memory[2] * 256);
		m_VoltageToSet = (float)(info.memory[3] / 10.0);
	}

	//if (m_EnableDrivingButton.GetCheck()) {
	ss.str(std::wstring());
	switch (info.cmdToSend.status) {
	case CmdStatus::FAILED:
		switch (info.state) {
		case RectifierState::FAILED_TO_SET_REMOTE_CONTROL:
			ss << "Failed to set remote control";
			break;
		case RectifierState::FAILED_TO_SET_VOLTAGE:
			ss << "Failed to set voltage";
			break;
		case RectifierState::FAILED_TO_STOP_EXECUTING_PROGRAMM:
			ss << "Failed to stop program executing";
			break;
		case RectifierState::FAILED_TO_START_EXECUTING_PROGRAMM:
			ss << "Failed to start program executing";
			break;
		default:
			ss << "Unknown error";
			break;
		}

		break;
	case CmdStatus::PREPARED:
		ss << "Setting voltage...";
		break;
	case CmdStatus::SUCCEEDED:
	case CmdStatus::EMPTY:
	{
		//				CString succeededValues(CA2T("Установлены: ", CP_UTF8));
		//				ss << succeededValues;
	}
	break;
	default:break;
	}
	ss.str(std::wstring());
	ss << " V: ";
	ss.setf(std::ios::floatfield, std::ios::fixed);
	ss << std::setprecision(1) << m_VoltageToSet;
	ss << " A: ";
	ss.setf(std::ios::floatfield, std::ios::fixed);
	ss << std::setprecision(1) << m_CurrentToSet;

	currentValue = ss.str().c_str();
	pDC->SelectObject(m_normalFont);
	pDC->SetTextColor(RGB(0, 255, 0));
	pDC->TextOutW(270, yShift + 16 + 1 * m_fontHeight, currentValue);
	//m_SetVoltage.SetReadOnly(FALSE);
	//m_SetVoltage.EnableWindow();
	//m_SetCurrent.SetReadOnly(FALSE);
	//m_SetCurrent.EnableWindow();
//}else {
	//m_SetVoltage.SetReadOnly();
	//m_SetVoltage.EnableWindow(FALSE);
	//m_SetCurrent.SetReadOnly();
	//m_SetCurrent.EnableWindow(FALSE);
//}

//CFont defaultFont;
//defaultFont.CreateFont(20, 0, 0, 0, 600, FALSE, FALSE, 0, RUSSIAN_CHARSET,
//	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
//	ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
//	L"Arial");
	 
	if ((info.stateF05.getControlByte() & 0x08) == 0x08) {
		// blocked by out chain
		pDC->DrawIcon(700, 75, aIcons[1]);
	}
	else if ((info.stateF05.getControlByte() & 0x04) == 0x04) {
		pDC->DrawIcon(700, 75, aIcons[2]);
	}
	else if ((info.stateF05.getControlByte() & 0x08) != 0x08) {
		pDC->DrawIcon(700, 75, aIcons[0]);
	}
	lock.Unlock();
}

void CRectifierControlView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CString caption(CA2T("Изменить значения", CP_UTF8));
	int yPos = 75;
	m_EnableDrivingButton.Create(caption, BS_ICON | WS_CHILD | WS_VISIBLE, CRect(10, yPos, 270, yPos + 30), this, ID_OPEN_SET_PARAMETERS_DIALOG);
	m_EnableDrivingButton.SetFont(mp_Font);
	CDC * pDC = this->GetDC();
	pDC->SelectObject(mp_Font);
	int xPos = 500;
	int buttomsSize = 80;
	CString start(CA2T("Пуск", CP_UTF8));
	m_StartButton.Create(start, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, 
		CRect(xPos, yPos, xPos + buttomsSize, yPos + 30), this, ID_START_PROGRAM);
	m_StartButton.SetFont(mp_Font);

	CString stop(CA2T("Стоп", CP_UTF8));
	m_StopButton.Create(stop, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_DISABLED,
		CRect(xPos + buttomsSize + 10, yPos, xPos + 10 + 2* buttomsSize, yPos + 30), this, ID_STOP_PROGRAM);
	m_StopButton.SetFont(mp_Font);

	xPos += 50;
	m_stateIcon.Create(L"Blocked", SS_ICON,
		CRect(xPos + buttomsSize + 10, yPos, xPos + 10 + 2 * buttomsSize, yPos + 30), this, ID_STATE_ICON | WS_VISIBLE);
	m_stateIcon.SetIcon(aIcons[0]);

}

void CRectifierControlView::OnOpenSetParametersDialog()
{
	// TODO: Add your command handler code here
	//if (BST_CHECKED == this->m_EnableDrivingButton.GetCheck()) {
	SetParametersDialog setParametersDialog(mp_Font, m_VoltageToSet, m_CurrentToSet);
	//setParametersDialog.SetFont(mp_Font);
	//setParametersDialog.setCurrentlyUsedComport(initComport);
	INT_PTR res = setParametersDialog.DoModal();
	if (IDOK == res) {
		std::wstringstream ss;
		ss << setParametersDialog.getVoltage();
		CString val = ss.str().c_str();
		//m_SetVoltage.SetWindowTextW(val);
		ss.str(std::wstring());
		ss << setParametersDialog.getCurrent();
		val = ss.str().c_str();
		//m_SetCurrent.SetWindowTextW(val);
		doControl = true;
		CRectifierControlDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (!pDoc)
			return;

		int rectifierID = pDoc->getRectifierInfo().id;
		std::map<int, RectifierInfo> & actualRectifiesInfos = theApp.getRectifierInfos();
		RectifierInfo & info = actualRectifiesInfos.at(rectifierID);
		CSingleLock lock(&info.cs, true);
		// set command to set voltage and current
		CmdToExecute cmdToExecute;

		cmdToExecute.rectifierCmd = RectifierCmd::SET_VOLTAGE_AND_CURRENT;
		cmdToExecute.voltage = setParametersDialog.getVoltage();
		cmdToExecute.current = setParametersDialog.getCurrent();
		info.cmdToSend = cmdToExecute;
		info.cmdToSend.status = CmdStatus::PREPARED; // prepared
		lock.Unlock();
	}
	else
	{
		this->m_EnableDrivingButton.SetCheck(0);
	}
	//}
}

void CRectifierControlView::execCommand(RectifierCmd rectifierCmd) {
	CRectifierControlDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	int rectifierID = pDoc->getRectifierInfo().id;
	std::map<int, RectifierInfo> & actualRectifiesInfos = theApp.getRectifierInfos();
	RectifierInfo & info = actualRectifiesInfos.at(rectifierID);
	CSingleLock lock(&info.cs, true);
	// set command to set voltage and current
	if (info.cmdToSend.status != CmdStatus::PREPARED
		&& info.cmdToSend.status != CmdStatus::SENT) {
		CmdToExecute cmdToExecute;
		cmdToExecute.rectifierCmd = rectifierCmd;
		info.cmdToSend = cmdToExecute;
		info.cmdToSend.status = CmdStatus::PREPARED; // prepared
	}
	lock.Unlock();
}

void CRectifierControlView::OnStartProgram()
{
	execCommand(RectifierCmd::START_EXISTIN_PROGRAM);
}

void CRectifierControlView::OnStopProgram()
{
	execCommand(RectifierCmd::STOP_EXISTIN_PROGRAM);
}

BOOL CRectifierControlView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// подготовка по умолчанию
	return DoPreparePrinting(pInfo);
}

void CRectifierControlView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: добавьте дополнительную инициализацию перед печатью
}

void CRectifierControlView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: добавьте очистку после печати
}


// диагностика CRectifierControlView

#ifdef _DEBUG
void CRectifierControlView::AssertValid() const
{
	CView::AssertValid();
}

void CRectifierControlView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CRectifierControlDoc* CRectifierControlView::GetDocument() const // встроена неотлаженная версия
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRectifierControlDoc)));
	return (CRectifierControlDoc*)m_pDocument;
}
#endif //_DEBUG


// обработчики сообщений CRectifierControlView


BOOL CRectifierControlView::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class

	if (wParam == ID_OPEN_SET_PARAMETERS_DIALOG)
	{
		//open dialog to set voltage and current

	}
	return CView::OnCommand(wParam, lParam);
}


BOOL CRectifierControlView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class
	return CView::OnNotify(wParam, lParam, pResult);
}




afx_msg LRESULT CRectifierControlView::OnIdSetParametersButton(WPARAM wParam, LPARAM lParam)
{
	return 0;
}


afx_msg LRESULT CRectifierControlView::OnMyMessage(WPARAM wParam, LPARAM lParam)
{
	return 0;
}




//void CRectifierControlView::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
//{
//	CView::OnActivate(nState, pWndOther, bMinimized);
//
//	// TODO: Add your message handler code here
//}


BOOL CRectifierControlView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return true;
	// return CView::OnEraseBkgnd(pDC);
}
