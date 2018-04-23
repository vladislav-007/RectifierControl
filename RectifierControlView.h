
// RectifierControlView.h : интерфейс класса CRectifierControlView
//

#pragma once


class CRectifierControlView : public CView
{
protected: // создать только из сериализации
	CRectifierControlView();
	DECLARE_DYNCREATE(CRectifierControlView)

// Атрибуты
public:
	CRectifierControlDoc* GetDocument() const;

// Операции
public:

// Переопределение
public:
	virtual void OnDraw(CDC* pDC);  // переопределено для отрисовки этого представления
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Реализация
public:
	virtual ~CRectifierControlView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Созданные функции схемы сообщений
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // отладочная версия в RectifierControlView.cpp
inline CRectifierControlDoc* CRectifierControlView::GetDocument() const
   { return reinterpret_cast<CRectifierControlDoc*>(m_pDocument); }
#endif

