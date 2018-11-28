
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

#include <sstream>

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
END_MESSAGE_MAP()

// создание/уничтожение CRectifierControlView

CRectifierControlView::CRectifierControlView()
{
	// TODO: добавьте код создания

}

CRectifierControlView::~CRectifierControlView()
{
}

BOOL CRectifierControlView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: изменить класс Window или стили посредством изменения
	//  CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// рисование CRectifierControlView

void CRectifierControlView::OnDraw(CDC* pDC)
{
	CRectifierControlDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: добавьте здесь код отрисовки для собственных данных
	CString rectifierName(CA2T("Выпрямитель: ", CP_UTF8));
	// MessageBox(rectifierName);
	CFont font;
	int fontHeight = 32;
	font.CreateFont(fontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	pDC->SelectObject(font);
	pDC->SetTextColor(RGB(0, 0, 0));
	int rectifierID = pDoc->getRectifierInfo().id;
	rectifierName += pDoc->getRectifierInfo().name;
	rectifierName += "(";
	rectifierName += pDoc->getRectifierInfo().comport;
	rectifierName += ")";
	std::map<int, RectifierInfo> & actualRectifiesInfos = theApp.getRectifierInfos();
	pDC->TextOutW(10, 10, rectifierName);
	CString rectifierState(CA2T("Состояние выпрямителя: ", CP_UTF8));
	std::wstringstream ss;
	const RectifierInfo & info = actualRectifiesInfos.at(rectifierID);
	rectifierState += toString(info.state);
	pDC->TextOutW(10, 10 + fontHeight, rectifierState);
	CString recivedData(CA2T("Напряжение: ", CP_UTF8));
	CFont headerFont;
	headerFont.CreateFont(2 * fontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	pDC->SelectObject(headerFont);
	pDC->TextOutW(10, 10 + 2 * fontHeight, recivedData);
	ss.str(std::wstring());
	
	ss << info.stateF07.V / 10.0;
	ss << " V";
	recivedData = ss.str().c_str();
	
	CFont valuesFont;
	valuesFont.CreateFont(2 * fontHeight, 0, 0, 0, 600, FALSE, FALSE, 0, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS,
		L"Arial");
	pDC->SelectObject(valuesFont);
	pDC->SetTextColor(RGB(0,0,255));
	pDC->TextOutW(350, 10 + 2 * fontHeight, recivedData);

	pDC->SelectObject(headerFont);
	pDC->SetTextColor(RGB(0, 0, 0));
	CString currentData(CA2T("Ток: ", CP_UTF8));
	pDC->TextOutW(230, 10 + 4 * fontHeight, currentData);
	ss.str(std::wstring());
	ss << (info.stateF07.aHi * 255 + info.stateF07.aLow);
	ss << " A";
	currentData = ss.str().c_str();
	pDC->SelectObject(valuesFont);
	pDC->SetTextColor(RGB(0, 255, 0));
	pDC->TextOutW(350, 10 + 4 * fontHeight, currentData);
}


// печать CRectifierControlView

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

	return CView::OnCommand(wParam, lParam);
}


BOOL CRectifierControlView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class

	return CView::OnNotify(wParam, lParam, pResult);
}
