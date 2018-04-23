
// RectifierControlView.h : ��������� ������ CRectifierControlView
//

#pragma once


class CRectifierControlView : public CView
{
protected: // ������� ������ �� ������������
	CRectifierControlView();
	DECLARE_DYNCREATE(CRectifierControlView)

// ��������
public:
	CRectifierControlDoc* GetDocument() const;

// ��������
public:

// ���������������
public:
	virtual void OnDraw(CDC* pDC);  // �������������� ��� ��������� ����� �������������
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// ����������
public:
	virtual ~CRectifierControlView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ��������� ������� ����� ���������
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // ���������� ������ � RectifierControlView.cpp
inline CRectifierControlDoc* CRectifierControlView::GetDocument() const
   { return reinterpret_cast<CRectifierControlDoc*>(m_pDocument); }
#endif

