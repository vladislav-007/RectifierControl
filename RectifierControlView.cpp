
// RectifierControlView.cpp : реализаци€ класса CRectifierControlView
//

#include "stdafx.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позвол€ет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "RectifierControl.h"
#endif

#include "RectifierControlDoc.h"
#include "RectifierControlView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRectifierControlView

IMPLEMENT_DYNCREATE(CRectifierControlView, CView)

BEGIN_MESSAGE_MAP(CRectifierControlView, CView)
	// —тандартные команды печати
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// создание/уничтожение CRectifierControlView

CRectifierControlView::CRectifierControlView()
{
	// TODO: добавьте код создани€

}

CRectifierControlView::~CRectifierControlView()
{
}

BOOL CRectifierControlView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: изменить класс Window или стили посредством изменени€
	//  CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// рисование CRectifierControlView

void CRectifierControlView::OnDraw(CDC* /*pDC*/)
{
	CRectifierControlDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: добавьте здесь код отрисовки дл€ собственных данных
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

CRectifierControlDoc* CRectifierControlView::GetDocument() const // встроена неотлаженна€ верси€
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRectifierControlDoc)));
	return (CRectifierControlDoc*)m_pDocument;
}
#endif //_DEBUG


// обработчики сообщений CRectifierControlView
