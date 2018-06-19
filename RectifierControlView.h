
// RectifierControlView.h : интерфейс класса CRectifierControlView
//

#pragma once


class CRectifierControlView : public CView
{
protected: // создать только из сериализации
	CRectifierControlView();
	DECLARE_DYNCREATE(CRectifierControlView)

// јтрибуты
public:
	CRectifierControlDoc* GetDocument() const;

// ќперации
public:

// ѕереопределение
public:
	virtual void OnDraw(CDC* pDC);  // переопределено дл¤ отрисовки этого представлени¤
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// –еализаци¤
public:
	virtual ~CRectifierControlView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// —озданные функции схемы сообщений
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // отладочна¤ верси¤ в RectifierControlView.cpp
inline CRectifierControlDoc* CRectifierControlView::GetDocument() const
   { return reinterpret_cast<CRectifierControlDoc*>(m_pDocument); }
#endif

