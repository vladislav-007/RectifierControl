
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
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
public:
	virtual void OnInitialUpdate();
private:
	// Data member in your dialog class 
	//CStatic m_V;
	//CStatic m_A;
	CButton m_EnableDrivingButton;
	CButton m_StartButton;
	CButton m_StopButton;
	//CStatic m_SetVoltage;
	CFont * mp_Font;
	CFont * m_normalFont;
	int m_fontHeight;
	//CStatic m_SetCurrent;
	bool doControl;
	float m_Voltage;
	float m_Current;
	float m_VoltageToSet;
	float m_CurrentToSet;

protected:
	afx_msg LRESULT OnIdSetParametersButton(WPARAM wParam, LPARAM lParam);
public:
//	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
protected:
	afx_msg LRESULT OnMyMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void execCommand(RectifierCmd rectifierCmd);
public:
	afx_msg void OnOpenSetParametersDialog();
	afx_msg void OnStartProgram();
	afx_msg void OnStopProgram();
//	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
};

#ifndef _DEBUG  // отладочна¤ верси¤ в RectifierControlView.cpp
inline CRectifierControlDoc* CRectifierControlView::GetDocument() const
   { return reinterpret_cast<CRectifierControlDoc*>(m_pDocument); }
#endif

