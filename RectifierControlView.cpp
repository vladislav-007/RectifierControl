
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
END_MESSAGE_MAP()

// создание/уничтожение CRectifierControlView

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


void CRectifierControlView::OnDraw(CDC* pDC)
{
	
	CRectifierControlDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

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

	CString overheatMsg(CA2T("Перегрев", CP_UTF8));
	CString protectMsg(CA2T("Защита", CP_UTF8));
	COLORREF overheatColor = RGB(250, 0, 100);
	COLORREF protectColor = RGB(230, 100, 100);
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
	pDC->SetTextColor(RGB(0,0,255));
	pDC->TextOutW(80, yShift + 10 + 2 * m_fontHeight, voltageValue);

	pDC->SelectObject(headerFont);
	pDC->SetTextColor(RGB(0, 0, 0));
	CString currentValue(CA2T("A: ", CP_UTF8));
	pDC->TextOutW(10, yShift + 10 + 4 * m_fontHeight, currentValue);
	ss.str(std::wstring());
	
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
	if (info.cmdToSend.rectifierCmd == RectifierCmd::SET_VOLTAGE_AND_CURRENT
		&& CmdStatus::SUCCEEDED == info.cmdToSend.status) {
		m_VoltageToSet = info.cmdToSend.voltage;
		m_CurrentToSet = info.cmdToSend.current;
	}
	
	//if (m_EnableDrivingButton.GetCheck()) {
		ss.str(std::wstring());
		switch(info.cmdToSend.status) {
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
				ss << " V: ";
				ss.setf(std::ios::floatfield, std::ios::fixed);
				ss << std::setprecision(1) << m_VoltageToSet;
				ss << " A: ";
				ss.setf(std::ios::floatfield, std::ios::fixed);
				ss << std::setprecision(1) << m_CurrentToSet;
			}
				break;
		default:break;
		}
		currentValue = ss.str().c_str();
		pDC->SelectObject(m_normalFont);
		pDC->SetTextColor(RGB(0, 255, 0));
		pDC->TextOutW(250, yShift + 16 + 1 * m_fontHeight, currentValue);
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

}

void CRectifierControlView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	
	CString caption(CA2T("Задать значения:", CP_UTF8));
	int yPos = 75;
	m_EnableDrivingButton.Create(caption, BS_ICON | WS_CHILD | WS_VISIBLE, CRect(10, yPos, 270, yPos + 30), this, ID_OPEN_SET_PARAMETERS_DIALOG);
	m_EnableDrivingButton.SetFont(mp_Font);
	CDC * pDC = this->GetDC();
	pDC->SelectObject(mp_Font);
	//pDC->SetTextColor(RGB(0, 255, 0));

	//m_V.Create(L"V:", WS_CHILD | WS_VISIBLE, CRect(270, yPos, 295, yPos + 30), this, 4);
	//m_V.SetFont(mp_Font);
	//m_SetVoltage.Create(L"-", WS_CHILD | WS_VISIBLE, CRect(300, 75, 400, 105), this, 2);
	////m_SetVoltage.SetReadOnly();
	//m_SetVoltage.SetFont(mp_Font);

	//m_A.Create(L"A:", WS_CHILD | WS_VISIBLE, CRect(420, yPos, 445, yPos + 30), this, 5);
	//m_A.SetFont(mp_Font);
	//m_SetCurrent.Create(L"-", WS_CHILD | WS_VISIBLE, CRect(450, 75, 550, 105), this, 3);
	////m_SetCurrent.SetReadOnly();
	//m_SetCurrent.SetFont(mp_Font);
	// TODO: Add your specialized code here and/or call the base class
}

void CRectifierControlView::OnOpenSetParametersDialog()
{
	// TODO: Add your command handler code here
	//if (BST_CHECKED == this->m_EnableDrivingButton.GetCheck()) {
		SetParametersDialog setParametersDialog(mp_Font, m_Voltage, m_Current);
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
			// set command to set voltage and current
			CmdToExecute cmdToExecute;
			
			cmdToExecute.rectifierCmd = RectifierCmd::SET_VOLTAGE_AND_CURRENT;
			cmdToExecute.voltage = setParametersDialog.getVoltage();
			cmdToExecute.current = setParametersDialog.getCurrent();
			info.cmdToSend = cmdToExecute;
			info.cmdToSend.status = CmdStatus::PREPARED; // prepared
		}
		else
		{
			this->m_EnableDrivingButton.SetCheck(0);
		}
	//}
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


